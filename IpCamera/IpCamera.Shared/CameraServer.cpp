#include "pch.h"
#include "Connection.h"
#include "CameraServer.h"

using namespace concurrency;
using namespace IpCamera;
using namespace MediaCaptureReader;
using namespace Platform;
using namespace std;
using namespace Windows::Foundation;
using namespace Windows::Media::Capture;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

IAsyncOperation<CameraServer^>^ CameraServer::CreateAsync(int port)
{
    auto server = ref new CameraServer();

    Trace("@%p creating CameraServer on port %i", (void*)server, port);

    return create_async([server, port]()
    {
        auto settings = ref new MediaCaptureInitializationSettings();
        settings->StreamingCaptureMode = StreamingCaptureMode::Video;

        Agile<MediaCapture> capture(ref new MediaCapture());
        return create_task(capture->InitializeAsync(settings)).then([capture]
        {
            return MediaReader::CreateFromMediaCaptureAsync(
                capture.Get(),
                AudioInitialization::Deselected,
                VideoInitialization::Nv12
                );
        }).then([server, port](MediaReader^ reader)
        {
            Trace("@%p created MediaReader @%p", (void*)server, (void*)reader);
            server->m_camera = reader;

            server->StartReadingFrame();

            return server->m_listener->BindServiceNameAsync(port.ToString());
        }).then([server, port]()
        {
            Trace("@%p bound socket listener to port %S", (void*)server, server->m_listener->Information->LocalPort->Data());
            return server;
        });
    });
}

CameraServer::CameraServer()
    : m_listener(ref new StreamSocketListener())
{
    m_connectionReceivedToken = m_listener->ConnectionReceived += 
        ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(
        this, 
        &CameraServer::OnConnectionReceived
        );
}

CameraServer::~CameraServer()
{
    auto lock = m_lock.LockExclusive();

    m_connections.clear();

    m_listener->ConnectionReceived -= m_connectionReceivedToken;
    delete m_listener; // calls IClosable::Close()
    m_listener = nullptr;

    delete m_camera;
    m_camera = nullptr;
}

void CameraServer::OnConnectionReceived(_In_ StreamSocketListener^ sender, _In_ StreamSocketListenerConnectionReceivedEventArgs^ e)
{
    auto lock = m_lock.LockExclusive();

    Trace("@%p new connection received: socket @%p", (void*)this, (void*)e->Socket);

    m_connections.push_back(make_unique<Connection>(e->Socket));
}

void CameraServer::StartReadingFrame()
{
    MediaReader^ camera;
    {
        auto lock = m_lock.LockExclusive();
        if (m_camera == nullptr)
        {
            return;
        }
        camera = m_camera;
    }

    auto stream = ref new InMemoryRandomAccessStream();

    create_task(camera->VideoStream->ReadAsync()).then([this, stream](MediaReaderReadResult^ result)
    {
        if (result->EndOfStream || result->Error)
        {
            Trace("@%p stopping streaming: EndOfStream %i, Error %i", (void*)this, result->EndOfStream, result->Error);
            return;
        }

        if (result->Sample == nullptr)
        {
            Trace("@%p null sample received, trying to read another frame", (void*)this);
            StartReadingFrame();
            return;
        }

        auto frame = safe_cast<MediaSample2D^>(result->Sample);
        Trace("@%p frame received @%p time %ims, duration %ims", 
            (void*)this, 
            (void*)frame, 
            (int)(frame->Timestamp.Duration / 10000), 
            (int)(frame->Duration.Duration / 10000)
            );

        create_task(ImageEncoder::SaveToStreamAsync(frame, stream, ImageCompression::Jpeg)).then([this, stream]
        {
            Trace("@%p frame encoded into JPEG stream of length %iB", (void*)this, (int)stream->Size);

            auto buffer = ref new Buffer((unsigned int)stream->Size);

            stream->Seek(0);
            return stream->ReadAsync(buffer, buffer->Capacity, InputStreamOptions::None);
        }).then([this](IBuffer^ buffer)
        {
            buffer = CreateHttpPartFromJpegBuffer(buffer);
            DispatchHttpPart(buffer);
            StartReadingFrame();
        });
    });
}

IBuffer^ CameraServer::CreateHttpPartFromJpegBuffer(_In_ IBuffer^ buffer)
{
    ostringstream partHeader;
    partHeader << "Content-Type: image/jpeg\r\nContent-length: " << buffer->Length << "\r\n\r\n";

    const char partFooter[] = "\r\n--CameraServerBoundary\r\n";

    auto part = ref new Buffer(partHeader.str().length() + buffer->Length + sizeof(partFooter) - 1);
    part->Length = part->Capacity;

    unsigned char *data = GetData(part);
    (void)memcpy(data, partHeader.str().c_str(), partHeader.str().length());
    (void)memcpy(data + partHeader.str().length(), GetData(buffer), buffer->Length);
    (void)memcpy(data + partHeader.str().length() + buffer->Length, partFooter, sizeof(partFooter) - 1);

    Trace("@%p created HTTP part @%p of length %iB", (void*)this, (void*)part, part->Length);

    return part;
}

void CameraServer::DispatchHttpPart(_In_ IBuffer^ buffer)
{
    Trace("@%p dispatching HTTP part @%p", (void*)this, (void*)buffer);

    auto lock = m_lock.LockExclusive();

    for (unique_ptr<Connection>& connection : m_connections)
    {
        connection->NotifyNewHttpPart(buffer);
    }
}

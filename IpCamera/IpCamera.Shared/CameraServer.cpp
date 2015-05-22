#include "pch.h"
#include "Connection.h"
#include "CameraServer.h"

using namespace concurrency;
using namespace IpCamera;
using namespace MediaCaptureReader;
using namespace Platform;
using namespace Platform::Collections;
using namespace std;
using namespace Windows::Foundation;
using namespace Windows::Media::Capture;
using namespace Windows::Networking;
using namespace Windows::Networking::Connectivity;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

IAsyncOperation<CameraServer^>^ CameraServer::CreateAsync(int port)
{
    Trace("@ creating CameraServer on port %i", port);

    return create_async([port]()
    {
        auto settings = ref new MediaCaptureInitializationSettings();
        settings->StreamingCaptureMode = StreamingCaptureMode::Video;

        Agile<MediaCapture> capture(ref new MediaCapture());
        return create_task(capture->InitializeAsync(settings)).then([capture, port]
        {
            return CreateFromMediaCaptureAsync(port, capture.Get());
        });
    });
}

IAsyncOperation<CameraServer^>^ CameraServer::CreateFromMediaCaptureAsync(int port, MediaCapture^ camera)
{
    CHKNULL(camera);

    auto server = ref new CameraServer();

    Trace("@%p creating CameraServer on port %i with camera @%p", (void*)server, port, camera);

    Agile<MediaCapture> capture(camera);
    return create_async([server, capture, port]()
    {
        return create_task(MediaReader::CreateFromMediaCaptureAsync(
                capture.Get(),
                AudioInitialization::Deselected,
                VideoInitialization::Nv12
                )).then([server, port](MediaReader^ reader)
        {
            Trace("@%p created MediaReader @%p", (void*)server, (void*)reader);
            server->m_camera = reader;

            server->StartReadingFrame();

            return server->m_listener->BindServiceNameAsync(port == 0 ? L"" : port.ToString());
        }).then([server]()
        {
            server->m_port = _wtoi(server->m_listener->Information->LocalPort->Data());
            if (server->m_port == 0)
            {
                throw ref new InvalidArgumentException(L"Failed to convert TCP port");
            }
            Trace("@%p bound socket listener to port %i", (void*)server, server->m_port);

            auto ipAddresses = ref new Vector<IPAddress^>();
            for (HostName^ host : NetworkInformation::GetHostNames())
            {
                if ((host->Type == HostNameType::Ipv4) || (host->Type == HostNameType::Ipv6))
                {
                    Trace("@%p network IP %S %S", (void*)server, host->Type.ToString()->Data(), host->CanonicalName->Data());
                    ipAddresses->Append(ref new IPAddress(host->Type, host->CanonicalName));
                }
            }
            server->m_ipAddresses = ipAddresses->GetView();

            return server;
        });
    });
}

CameraServer::CameraServer()
    : m_listener(ref new StreamSocketListener())
    , m_port(-1)
{
    m_listener->ConnectionReceived += 
        ref new TypedEventHandler<StreamSocketListener^, StreamSocketListenerConnectionReceivedEventArgs^>(
        this, 
        &CameraServer::OnConnectionReceived
        );
}

CameraServer::~CameraServer()
{
    auto lock = m_lock.LockExclusive();

    m_connections.clear();

    // Don't remove the m_listener->ConnectionReceived event handler here: it throws

    delete m_listener; // calls IClosable::Close()
    m_listener = nullptr;

    delete m_camera;
    m_camera = nullptr;
}

void CameraServer::OnConnectionReceived(_In_ StreamSocketListener^ sender, _In_ StreamSocketListenerConnectionReceivedEventArgs^ e)
{
    auto lock = m_lock.LockExclusive();
    if (__abi_disposed)
    {
        return;
    }

    Trace("@%p new connection received: socket @%p", (void*)this, (void*)e->Socket);

    m_connections.push_back(make_unique<Connection>(e->Socket));
}

void CameraServer::StartReadingFrame()
{
    MediaReader^ camera;
    {
        auto lock = m_lock.LockExclusive();
        if (__abi_disposed)
        {
            return;
        }
        camera = m_camera;
    }

    auto stream = ref new InMemoryRandomAccessStream();

    task_from_result().then([camera]()
    {
        return camera->VideoStream->ReadAsync();
    }).then([this, stream](MediaReaderReadResult^ result)
    {
        if (result->EndOfStream || result->Error)
        {
            Trace("@%p stopping streaming: EndOfStream %i, Error %i", (void*)this, result->EndOfStream, result->Error);
            return task_from_result();
        }

        if (result->Sample == nullptr)
        {
            Trace("@%p null sample received, trying to read another frame", (void*)this);
            StartReadingFrame();
            return task_from_result();
        }

        auto frame = safe_cast<MediaSample2D^>(result->Sample);
        Trace("@%p frame received @%p time %ims, duration %ims", 
            (void*)this, 
            (void*)frame, 
            (int)(frame->Timestamp.Duration / 10000), 
            (int)(frame->Duration.Duration / 10000)
            );

        return create_task(ImageEncoder::SaveToStreamAsync(frame, stream, ImageCompression::Jpeg)).then([this, stream]
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
    }).then([this](task<void> t)
    {
        try
        {
            t.get();
        }
        catch (Exception^ e)
        {
            this->Failed(this, ref new CameraServerFailedEventArgs(e));
        }
    });

}

IBuffer^ CameraServer::CreateHttpPartFromJpegBuffer(_In_ IBuffer^ buffer)
{
    ostringstream partHeader;
    partHeader << "Content-Type: image/jpeg\r\nContent-length: " << buffer->Length << "\r\n\r\n";

    const char partFooter[] = "\r\n--CameraServerBoundary\r\n";

    auto part = ref new Buffer((unsigned int)(partHeader.str().length() + buffer->Length + sizeof(partFooter) - 1));
    part->Length = part->Capacity;

    unsigned char *data = GetData(part);
    (void)memcpy(data, partHeader.str().c_str(), partHeader.str().length());
    (void)memcpy(data + partHeader.str().length(), GetData(buffer), buffer->Length);
    (void)memcpy(data + partHeader.str().length() + buffer->Length, partFooter, sizeof(partFooter) - 1);

    Trace("@ created HTTP part @%p of length %iB", (void*)part, part->Length);

    return part;
}

void CameraServer::DispatchHttpPart(_In_ IBuffer^ buffer)
{
    auto lock = m_lock.LockExclusive();
    if (__abi_disposed)
    {
        return;
    }

    Trace("@%p dispatching HTTP part @%p", (void*)this, (void*)buffer);

    for (unique_ptr<Connection>& connection : m_connections)
    {
        connection->NotifyNewHttpPart(buffer);
    }
}

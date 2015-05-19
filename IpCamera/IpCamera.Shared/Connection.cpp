#include "pch.h"
#include "Connection.h"

using namespace concurrency;
using namespace Platform;
using namespace Windows::Networking::Sockets;
using namespace Windows::Storage::Streams;

 // Pre-canned response sent to HTTP clients
const char s_responseHeader[] =
    "HTTP/1.1 200 OK\r\n"
    "Cache-Control: no-cache\r\n"
    "Pragma: no-cache\r\n"
    "Expires: Thu, 01 Dec 1994 16:00:00 GMT\r\n"
    "Connection: close\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=CameraServerBoundary\r\n"
    "\r\n"
    "--CameraServerBoundary\r\n";

Connection::Connection(_In_ StreamSocket^ socket)
    : m_acceptingData(false)
    , m_closed(false)
    , m_socket(socket)
{
    NT_ASSERT(socket != nullptr);

    task_from_result().then([socket]()
    {
        auto requestBuffer = ref new Buffer(4096);
        return socket->InputStream->ReadAsync(requestBuffer, requestBuffer->Capacity, InputStreamOptions::Partial);
    }).then([this, socket](IBuffer^)
    {
        Trace("@%p received HTTP request from socket @%p", (void*)this, (void*)m_socket);

        auto responseHeaderBuffer = ref new Buffer(sizeof(s_responseHeader) - 1);
        (void)memcpy(GetData(responseHeaderBuffer), s_responseHeader, responseHeaderBuffer->Capacity);
        responseHeaderBuffer->Length = responseHeaderBuffer->Capacity;

        Trace("@%p sending HTTP response header to socket @%p: %iB", (void*)this, (void*)m_socket, responseHeaderBuffer->Length);

        return socket->OutputStream->WriteAsync(responseHeaderBuffer);
    }).then([this, socket](unsigned int)
    {
        return socket->OutputStream->FlushAsync();
    }).then([this](bool)
    {
        auto lock = m_lock.LockExclusive();
        if (!m_closed)
        {
            m_acceptingData = true;
        }
    }).then([this](task<void> t)
    {
        try
        {
            t.get();
        }
        catch (...)
        {
            this->Close();
        }
    });
}

Connection::~Connection()
{
    delete m_socket; // calls IClosable::Close()
}

void Connection::Close()
{
    auto lock = m_lock.LockExclusive();

    m_closed = true;
    m_acceptingData = false;

    delete m_socket; // calls IClosable::Close()
    m_socket = nullptr;
}

void Connection::NotifyNewHttpPart(_In_ IBuffer^ buffer)
{
    IOutputStream^ stream;
    {
        auto lock = m_lock.LockExclusive();

        if (m_closed || !m_acceptingData)
        {
            Trace("@%p dropping HTTP part @%p: closed %i, acceptingData %i", (void*)this, (void*)buffer, m_closed, m_acceptingData);
            return;
        }
        m_acceptingData = false;

        stream = m_socket->OutputStream;
    }

    task_from_result().then([stream, buffer]()
    {
        return stream->WriteAsync(buffer);
    }).then([this, stream](unsigned int)
    {
        return stream->FlushAsync();
    }).then([this, buffer](bool)
    {
        Trace("@%p completed sending HTTP part @%p", (void*)this, (void*)buffer);

        auto lock = m_lock.LockExclusive();
        if (!m_closed)
        {
            m_acceptingData = true;
        }
    }).then([this](task<void> t)
    {
        try
        {
            t.get();
        }
        catch (...)
        {
            this->Close();
        }
    });
}
#pragma once

class Connection
{
public:

    Connection(_In_ Windows::Networking::Sockets::StreamSocket^ socket);
    ~Connection();

    void NotifyNewHttpPart(_In_ Windows::Storage::Streams::IBuffer^ buffer);

    void Close();

private:

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    Windows::Networking::Sockets::StreamSocket^ m_socket;
    bool m_acceptingData;
    bool m_closed;

    Microsoft::WRL::Wrappers::SRWLock m_lock;
};


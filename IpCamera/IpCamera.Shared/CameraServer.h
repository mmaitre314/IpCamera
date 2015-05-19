#pragma once

namespace IpCamera
{
    public ref class CameraServerFailedEventArgs sealed
    {
    public:

        property int ErrorCode { int get(){ return m_errorCode; } }
        property Platform::String^ Message { Platform::String^ get(){ return m_message; } }

    internal:

        CameraServerFailedEventArgs(_In_ Platform::Exception^ e)
            : m_errorCode(e->HResult)
            , m_message(e->Message)
        {
        }

    private:

        int m_errorCode;
        Platform::String^ m_message;
    };

    public ref class CameraServer sealed
    {
    public:

        ///<summary>Creates a CameraServer object</summary>
        ///<param name="port">Number of the TCP port on which to listen for incoming connections</param>
        ///<remarks>
        ///The camera and network listener are started immediately.
        ///The call requires app capabilities: webcam and either internetClientServer or privateNetworkClientServer.
        ///</remarks>
        static Windows::Foundation::IAsyncOperation<CameraServer^>^ CreateAsync(int port);

        // IClosable
        virtual ~CameraServer();

        event Windows::Foundation::TypedEventHandler<Platform::Object^, CameraServerFailedEventArgs^>^ Failed;

    private:

        CameraServer();

        void OnConnectionReceived(
            _In_ Windows::Networking::Sockets::StreamSocketListener^ sender,
            _In_ Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ e
            );

        void StartReadingFrame();
        Windows::Storage::Streams::IBuffer^ CreateHttpPartFromJpegBuffer(_In_ Windows::Storage::Streams::IBuffer^ buffer);
        void DispatchHttpPart(_In_ Windows::Storage::Streams::IBuffer^ buffer);

        Windows::Networking::Sockets::StreamSocketListener^ m_listener;
        MediaCaptureReader::MediaReader^ m_camera;
        std::list<std::unique_ptr<Connection>> m_connections;

        Windows::Foundation::EventRegistrationToken m_connectionReceivedToken;

        Microsoft::WRL::Wrappers::SRWLock m_lock;
    };
}

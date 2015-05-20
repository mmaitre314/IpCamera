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
        ///<remarks>
        /// Both the camera and the TCP port are selected automatically.
        /// The camera and network listener are started immediately.
        /// The call requires app capabilities: webcam and either internetClientServer or privateNetworkClientServer.
        /// Frames are captured from the preview video stream.
        ///</remarks>
        static Windows::Foundation::IAsyncOperation<CameraServer^>^ CreateAsync()
        {
            return CreateAsync(0);
        }

        ///<summary>Creates a CameraServer object</summary>
        ///<param name="port">Number of the TCP port on which to listen for incoming connections</param>
        ///<remarks>
        /// The camera is selected automatically.
        /// The camera and network listener are started immediately.
        /// The call requires app capabilities: webcam and either internetClientServer or privateNetworkClientServer.
        /// Frames are captured from the preview video stream.
        ///</remarks>
        static Windows::Foundation::IAsyncOperation<CameraServer^>^ CreateAsync(int port);

        ///<summary>Creates a CameraServer object</summary>
        ///<param name="camera">The camera to record from</param>
        ///<remarks>
        /// The TCP port is selected automatically. The caller selects which camera to record from.
        /// The network listener is started immediately. The camera object must have been initialized before the call.
        /// The call requires app capabilities: either internetClientServer or privateNetworkClientServer.
        /// Frames are captured from the preview video stream.
        ///</remarks>
        static Windows::Foundation::IAsyncOperation<CameraServer^>^ CreateFromMediaCaptureAsync(Windows::Media::Capture::MediaCapture^ camera)
        {
            return CreateFromMediaCaptureAsync(0, camera);
        }

        ///<summary>Creates a CameraServer object</summary>
        ///<param name="port">Number of the TCP port on which to listen for incoming connections</param>
        ///<param name="camera">The camera to record from</param>
        ///<remarks>
        /// The network listener is started immediately. The camera object must have been initialized before the call.
        /// The call requires app capabilities: either internetClientServer or privateNetworkClientServer.
        /// Frames are captured from the preview video stream.
        ///</remarks>
        static Windows::Foundation::IAsyncOperation<CameraServer^>^ CreateFromMediaCaptureAsync(int port, Windows::Media::Capture::MediaCapture^ camera);

        // IClosable
        virtual ~CameraServer();

        ///<summary>Raised when the CameraServer object becomes non-functional</summary>
        event Windows::Foundation::TypedEventHandler<Platform::Object^, CameraServerFailedEventArgs^>^ Failed;

        ///<summary>TCP port CameraServer is listening on</summary>
        property int Port { int get() { return m_port; } }
        
    private:

        CameraServer();

        void OnConnectionReceived(
            _In_ Windows::Networking::Sockets::StreamSocketListener^ sender,
            _In_ Windows::Networking::Sockets::StreamSocketListenerConnectionReceivedEventArgs^ e
            );

        static Windows::Storage::Streams::IBuffer^ CreateHttpPartFromJpegBuffer(_In_ Windows::Storage::Streams::IBuffer^ buffer);

        void StartReadingFrame();
        void DispatchHttpPart(_In_ Windows::Storage::Streams::IBuffer^ buffer);

        Windows::Networking::Sockets::StreamSocketListener^ m_listener;
        MediaCaptureReader::MediaReader^ m_camera;
        std::list<std::unique_ptr<Connection>> m_connections;
        int m_port;

        Microsoft::WRL::Wrappers::SRWLock m_lock;
    };
}

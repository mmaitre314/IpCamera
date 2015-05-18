using IpCamera;
using MediaCaptureReader;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace IpCameraTestApp
{
    public sealed partial class MainPage : Page
    {
        CameraServer m_server; // Use a member variable to keep the server alive

        public MainPage()
        {
            this.InitializeComponent();
            this.NavigationCacheMode = NavigationCacheMode.Required;
        }

        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            // Start the camera and open a HTTP listener on port 31415
            m_server = await CameraServer.CreateAsync(31415);

            // Start an HTTP client, connect to port 31415, and display the video
            var client = await HttpMjpegCaptureSource.CreateFromUriAsync("http://localhost:31415/");

            // Log video playback events
            Preview.MediaFailed += Preview_MediaFailed;
            Preview.MediaEnded += Preview_MediaEnded;
            Preview.MediaOpened += Preview_MediaOpened;
            Preview.CurrentStateChanged += Preview_CurrentStateChanged;

            // Start playback
            Preview.SetMediaStreamSource(client.Source);
        }

        void Preview_CurrentStateChanged(object sender, RoutedEventArgs e)
        {
            Log.Text += "Preview_CurrentStateChanged: " + Preview.CurrentState.ToString() + "\n";
        }

        void Preview_MediaOpened(object sender, RoutedEventArgs e)
        {
            Log.Text += "Preview_MediaOpened\n";
        }

        void Preview_MediaEnded(object sender, RoutedEventArgs e)
        {
            Log.Text += "Preview_MediaEnded\n";
        }

        void Preview_MediaFailed(object sender, ExceptionRoutedEventArgs e)
        {
            Log.Text += "Preview_MediaFailed\n";
        }
    }
}

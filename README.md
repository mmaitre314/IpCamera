An API to start an HTTP server and stream the video from a camera on the local machine over a network in MJPEG format. 

```c#
// Start the camera and open an HTTP listener on port 31415
var server = await CameraServer.CreateAsync(31415);
```

The server supports multiple clients connected at the same time.

Using the [MediaCaptureReader](http://www.nuget.org/packages/MMaitre.MediaCaptureReader/) NuGet package client apps can display the video by creating an `HttpMjpegCaptureSource` and connecting it to `MediaElement`.

```xml
<MediaElement Name="Preview" AutoPlay="True" RealTimePlayback="True"/>
```
```c#
// Start an HTTP client, connect to port 31415, and display the video
var client = await HttpMjpegCaptureSource.CreateFromUriAsync("http://localhost:31415/");

// Start playback
Preview.SetMediaStreamSource(client.Source);
```

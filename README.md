[![Build status](https://ci.appveyor.com/api/projects/status/7ucks5y2hqo7mphg?svg=true)](https://ci.appveyor.com/project/mmaitre314/IpCamera)
[![NuGet package](http://mmaitre314.github.io/images/nuget.png)](https://www.nuget.org/packages/MMaitre.IpCamera/)
[![Symbols](http://mmaitre314.github.io/images/Symbols.png)](http://mmaitre314.github.io/2015/05/24/personal-pdb-symbol-server.html)

IP Camera
=========

An API to stream video from cameras on Windows and Windows Phone devices (and probably to any HTTP MJPEG video player).

![IpCamera](http://mmaitre314.github.io/images/IpCamera.jpg)

`CameraServer` starts an HTTP server and streams the video from a camera on the local machine over a network in MJPEG format. 
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

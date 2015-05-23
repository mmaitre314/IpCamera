using System;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using System.Threading.Tasks;
using MediaCaptureReader;
using IpCamera;

namespace UnitTests.NuGet.WindowsPhone
{
    [TestClass]
    public class UnitTests
    {
        [TestMethod]
        public async Task CS_WP_N_CreateFromMediaCaptureAsync()
        {
            Logger.LogMessage("Creating NullMediaCapture");
            var camera = NullMediaCapture.Create();
            await camera.InitializeAsync();

            Logger.LogMessage("Creating CameraServer");
            using (var server = await CameraServer.CreateFromMediaCaptureAsync(camera))
            {
                Assert.IsTrue(server.Port > 0);
                Assert.IsTrue(server.IPAddresses.Count >= 1);
            }
        }
    }
}

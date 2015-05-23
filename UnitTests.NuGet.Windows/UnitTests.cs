using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.TestPlatform.UnitTestFramework;
using IpCamera;
using MediaCaptureReader;
using System.Threading.Tasks;

namespace UnitTests.NuGet.Windows
{
    [TestClass]
    public class UnitTests
    {
        [TestMethod]
        public async Task CS_W_N_CreateFromMediaCaptureAsync()
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

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricUS.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System;
    using System.Collections.Generic;
    using System.Fabric.UpgradeService;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This is the class that tests the validation of the files that we deploy.
    /// </summary>
    [TestClass]
    internal class FabricUSTest
    {
        [TestCleanup]
        public void Cleanup()
        {
        }

        [TestMethod]
        [TestProperty("ThreadingModel", "MTA")]
        public async Task StreamChannelTest()
        {
            var mock = new SimulatedSFRP();
            var wrpStreamChannel = new WrpStreamChannel(
                new System.Uri("http://dummyUri"),
                Guid.NewGuid().ToString(),
                new DummyConfigStore(),
                string.Empty);

            wrpStreamChannel.MockTestHandler = mock;
            mock.MoveNext(new StreamRequest() { PingRequest = true });

            var token = new CancellationTokenSource(5 * 1000).Token;
            await wrpStreamChannel.StartStreamChannelNoThrowAsync<TaskCanceledException>(token);            
            var status = mock.VerifyResponse(1, null);

            if (!status)
            {
                Assert.IsTrue(status);
            }

            var request2 = new StreamRequest()
            {
                ClusterQueryUri = "http://ClusterQueryUri",
                Content = "TestContent",
                Headers = new Dictionary<string, string>() { { "a", "b" } },
                Method = "POST",
                RequestId = Guid.NewGuid().ToString(),
                RequestReceivedStamp = DateTime.Now,
                CharSet = "utf-8",
                MediaType = "text/plain"
            };

            mock.MoveNext(request2);

            token = new CancellationTokenSource(5 * 1000).Token;
            await wrpStreamChannel.StartStreamChannelNoThrowAsync<TaskCanceledException>(token);

            System.Threading.Thread.Sleep(TimeSpan.FromSeconds(5));

            status = mock.VerifyResponse(3, request2);

            if (!status)
            {
                Assert.IsTrue(status);
            }
        }
    }
}
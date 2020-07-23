// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using System.Threading.Tasks;

    [TestClass]
    public class ProgramTest
    {
        [TestMethod]
        public void WaitForProgramClosureTest()
        {
            Task.Run(() =>
            {
                Task.Delay(3000).Wait();
                ProcessCloser.ExitEvent.Set();
            });

            int status = Program.WaitForProgramClosure();

            Assert.AreEqual(status, -1, "Verifying if exit is because ProcessCloser.ExitEvent is set");

        }
    }
}
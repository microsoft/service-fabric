// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class RepairTaskExecutorDataTest
    {
        [TestMethod]
        public void JsonConversionTest()
        {
            var rted = new RepairTaskExecutorData
            {
                JobId = "id1",
                UD = 3                
            };

            var json = rted.ToJson();

            var rted2 = json.FromJson<RepairTaskExecutorData>();
            
            Assert.AreEqual(rted.JobId, rted2.JobId);
            Assert.AreEqual(rted.UD, rted2.UD);
        }
    }
}
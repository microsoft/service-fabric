// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using RD.Fabric.PolicyAgent;

    [TestClass]
    public class BondHelperTest
    {
        [TestMethod]
        public void ToAndFromBondObjectTest()
        {
            var rihi = new RoleInstanceHealthInfo
            {
                Health = RoleStateEnum.CreatingVM,
                RoleInstanceName = "ServiceFabricRole_IN_1",
            };

            byte[] payload = rihi.GetPayloadFromBondObject();
            var rihi2 = payload.GetBondObjectFromPayload<RoleInstanceHealthInfo>();

            Assert.AreEqual(rihi.Health, rihi2.Health);
            Assert.AreEqual(rihi.RoleInstanceName, rihi2.RoleInstanceName);

        }
    }
}
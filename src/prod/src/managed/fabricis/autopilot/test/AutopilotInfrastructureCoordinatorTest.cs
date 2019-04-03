// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot.Test
{
    using Autopilot;
    using InfrastructureService.Test;
    using Microsoft.VisualStudio.TestTools.UnitTesting;

    [TestClass]
    public class AutopilotInfrastructureCoordinatorTest
    {
        [TestMethod]
        public void MaintenanceRecordIdTest()
        {
            MaintenanceRecordId id;

            foreach (var rt in Enum.GetValues(typeof(RepairType)))
            {
                id = new MaintenanceRecordId("MACHINE1", rt.ToString());
                Assert.AreEqual("MACHINE1", id.MachineName);
                Assert.AreEqual(rt, id.RepairType);

                // case insensitivity
                id = new MaintenanceRecordId("machine1", rt.ToString().ToLowerInvariant());
                Assert.AreEqual("MACHINE1", id.MachineName);
                Assert.AreEqual(rt, id.RepairType);
            }

            // unknown repair type
            id = new MaintenanceRecordId("MACHINE1", "foo");
            Assert.AreEqual("MACHINE1", id.MachineName);
            Assert.AreEqual(RepairType.Unknown, id.RepairType);
        }

        [TestMethod]
        public void CoordinatorDutyCycleWarningThresholdTest()
        {
            var store = new MockConfigStore();
            var section = new ConfigSection(new TraceType("X"), store, "Section");

            var reader = new ConfigReader(section);

            Assert.AreEqual(0.5, reader.CoordinatorDutyCycleWarningThreshold);

            store.UpdateKey("Section", "Autopilot.CoordinatorDutyCycleWarningThreshold", "0.75");
            Assert.AreEqual(0.75, reader.CoordinatorDutyCycleWarningThreshold);

            store.UpdateKey("Section", "Autopilot.CoordinatorDutyCycleWarningThreshold", "1.75");
            Assert.AreEqual(1.0, reader.CoordinatorDutyCycleWarningThreshold);

            store.UpdateKey("Section", "Autopilot.CoordinatorDutyCycleWarningThreshold", "-2");
            Assert.AreEqual(0.0, reader.CoordinatorDutyCycleWarningThreshold);
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using Microsoft.WindowsAzure.ServiceRuntime.Management;

namespace System.Fabric.InfrastructureService.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using WEX.TestExecution;
    using System.Collections.Generic;
    using Linq;

    [TestClass]
    public class CoordinatorHelperTest
    {
        [TestMethod]
        public static void NodeNameTranslationTest()
        {
            Verify.AreEqual("ServiceFabricRole_IN_0".TranslateRoleInstanceToNodeName(), "ServiceFabricRole.0", "old style validation");
            Verify.AreEqual("_VMScaleSetServiceFabricRole_0".TranslateRoleInstanceToNodeName(), "_VMScaleSetServiceFabricRole_0", "VMSS validation. Same input as output");

            Verify.Throws<ArgumentNullException>(() =>
            {
                string n = null;
                n.TranslateRoleInstanceToNodeName();
            });

            Verify.Throws<ArgumentException>(() => { string.Empty.TranslateRoleInstanceToNodeName(); });
            Verify.Throws<ArgumentException>(() => { " ".TranslateRoleInstanceToNodeName(); });

            Verify.AreEqual("ServiceFabricRole.0".TranslateNodeNameToRoleInstance(), "ServiceFabricRole_IN_0", "old style validation");
            Verify.AreEqual("_VMScaleSetServiceFabricRole_0".TranslateNodeNameToRoleInstance(), "_VMScaleSetServiceFabricRole_0", "VMSS validation. Same input as output");

            Verify.Throws<ArgumentNullException>(() =>
            {
                string n = null;
                n.TranslateNodeNameToRoleInstance();
            });

            Verify.Throws<ArgumentException>(() => { string.Empty.TranslateNodeNameToRoleInstance(); });
            Verify.Throws<ArgumentException>(() => { " ".TranslateNodeNameToRoleInstance(); });
        }

        /// <summary>
        /// Tests the method <see cref="CoordinatorHelper.IsManualApprovalRequired"/>
        /// </summary>
        [TestMethod]
        public void IsManualApprovalRequiredTest()
        {
            bool manualApprovalRequired = CoordinatorHelper.IsManualApprovalRequired(null);

            Verify.AreEqual(manualApprovalRequired, false,
                "Verifying if <null> management notification returns false for 'IsManualApprovalRequired'");

            TestManualApproval(new List<ImpactReason> { ImpactReason.VendorRepairBegin, ImpactReason.Reboot }, true);
            TestManualApproval(new List<ImpactReason> { ImpactReason.VendorRepairEnd, ImpactReason.Reboot }, true);
            TestManualApproval(new List<ImpactReason> { ImpactReason.HostReboot, ImpactReason.Reboot }, false);
            TestManualApproval(new List<ImpactReason>(), false);
        }

        [TestMethod]
        public void ToManagementRoleInstanceStatusTest()
        {
            var managementRoleInstanceStatusValues = Enum.GetValues(typeof (ManagementRoleInstanceStatus));
            var roleInstanceStateValues = Enum.GetValues(typeof (RoleInstanceState));

            Verify.AreEqual(managementRoleInstanceStatusValues.Length, roleInstanceStateValues.Length);
            
            for (int i = 0; i < managementRoleInstanceStatusValues.Length; i++)
            {
                ManagementRoleInstanceStatus status = (ManagementRoleInstanceStatus)managementRoleInstanceStatusValues.GetValue(i);
                RoleInstanceState state = (RoleInstanceState)roleInstanceStateValues.GetValue(i);

                Verify.AreEqual(status.ToString(), state.ToString());
                Verify.AreEqual((int)status, (int)state);
            }
        }

        private static void TestManualApproval(IEnumerable<ImpactReason> impactReasons, bool expectedReturnValue)
        {
            var notification = new MockManagementNotificationContext
            {
                ImpactedInstances = new List<ImpactedInstance>
                {
                    new ImpactedInstance("id_A", impactReasons),
                }
            };

            bool manualApprovalRequired = notification.IsManualApprovalRequired();
            Verify.AreEqual(manualApprovalRequired, expectedReturnValue);
        }
    }
}
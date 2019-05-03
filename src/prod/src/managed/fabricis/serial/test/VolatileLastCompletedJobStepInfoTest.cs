// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using Threading;
    using WEX.TestExecution;

    [TestClass]
    public class VolatileLastCompletedJobStepInfoTest
    {
        [TestMethod]
        public void QualifiesForReAckTest()
        {
            Verify.AreEqual(
                new VolatileLastCompletedJobStepInfo(TimeSpan.FromMinutes(5)).QualifiesForReAck(
                    new MockManagementNotificationContext()), 
                false,
                "Re-ack not qualified since notification type is not CompleteJobStep");

            Verify.AreEqual(
                new VolatileLastCompletedJobStepInfo(TimeSpan.Zero).QualifiesForReAck(
                    new MockManagementNotificationContext { NotificationType = NotificationType.CompleteJobStep }),
                false,
                "Re-ack not qualified since an update was never done previously");

            {
                var info = new VolatileLastCompletedJobStepInfo(TimeSpan.Zero);
                var jobId = new Guid().ToString();
                const int ud = 0;

                info.Update(jobId, ud, NotificationType.CompleteJobStep, NotificationAction.SignalReady);

                Thread.Sleep(3000);

                bool qualifies = info.QualifiesForReAck(new MockManagementNotificationContext
                {
                    ActiveJobId = jobId,
                    ActiveJobStepTargetUD = ud,
                    NotificationType = NotificationType.CompleteJobStep,
                });

                Verify.AreEqual(qualifies, true,
                    "Re-ack qualified since update time is beyond threshold and notification type is CompleteJobStep");
            }
        }
    }
}
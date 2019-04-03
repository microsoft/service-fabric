// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Bond;
    using Collections.Generic;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using RD.Fabric.PolicyAgent;
    using Repair;

    [TestClass]
    public class CoordinatorHelperTest
    {
        [TestMethod]
        public void GetJobIdTest()
        {
            var id = Guid.NewGuid().ToString();
            uint ud = 2;

            var repairTask = new ClusterRepairTask
            {
                ExecutorData = new RepairTaskExecutorData { JobId = id, UD = ud }.ToJson(),
            };

            var wrapperRepairTask = new ServiceFabricRepairTask(repairTask);

            var parsedId = wrapperRepairTask.GetJobId();
            Assert.AreEqual(id, parsedId, "Verifying if job Id could be parsed from repair task");
        }

        [TestMethod]
        public void ToJsonTest()
        {
            var tji = new TenantJobInfo
            {
                DocumentIncarnation = 1
            };

            tji.Jobs.Add(
                new TenantJob
                {
                    ContextStringGivenByTenant = "tojsontest",
                    JobStatus = JobStatusEnum.Executing,
                    JobStep = new JobStepInfo
                    {
                        AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement,
                        ActionStatus = ImpactActionStatus.NotExecuted,
                        ImpactStep = ImpactStepEnum.ImpactStart,
                        DeadlineForResponse = "-1",
                        CurrentlyImpactedRoleInstances = new List<RoleInstanceImpactedByJob>
                        {
                            new RoleInstanceImpactedByJob
                            {
                                PreviousServiceInstanceName = "A",
                                UpdatedServiceInstanceName = "B",
                                RoleInstanceName = "SF_IN_1",
                                UpdateDomain = 1,
                            }
                        },
                    }
                });

            var doc = new PolicyAgentDocumentForTenant
            {
                Incarnation = "1",
                JobInfo = new Bonded<TenantJobInfo>(tji),
            };

            // simple tests for now
            var json = doc.ToJson();
            Assert.IsNotNull(json);

            // from json is not necessary for bond types since PolicyAgentDocumentForTenant contains an IBonded<JobInfo> which
            // cannot be deserialized in a straight forward fashion.            
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

//namespace System.Fabric.InfrastructureService.Parallel.Test
//{
//    using Collections;
//    using Collections.Generic;
//    using Common;
//    using InfrastructureService.Test;
//    using Linq;
//    using Microsoft.VisualStudio.TestTools.UnitTesting;
//    using RD.Fabric.PolicyAgent;
//    using Repair;
//    using WEX.TestExecution;

//    [TestClass]
//    public class ConfigTenantJobMapperTest
//    {
//        private static readonly IList<ImpactActionEnum> validImpactActions =
//            Enum.GetValues(typeof(ImpactActionEnum)).Cast<ImpactActionEnum>().Where(e => e != ImpactActionEnum.Unknown).ToList();

//        private const string ConfigSectionName = "ISTest1";

//        [TestMethod]
//        public void TenantJobMappingTest1()
//        {
//            var configStore = new MockConfigStore();
//            var configSection = new ConfigSection(Constants.TraceType, configStore, ConfigSectionName);
//            var policy = new ConfigTenantJobActionPolicy(configSection);

//            {
//                configStore.AddKeyValue(
//                    ConfigSectionName,
//                    ParallelJobs.Constants.ConfigKeys.MaxParallelJobCountTotal,
//                    validImpactActions.Count.ToString());

//                const int jobCountOfEachType = 3;
//                var mappedTenantJobs = CreateMappedTenantJobs(jobCountOfEachType);
//                policy.ApplyAsync(Guid.NewGuid(), mappedTenantJobs, new Dictionary<string, IMappedRepairTask>());

//                Verify.AreEqual(
//                    mappedTenantJobs.Where(e => e.Value.Actions.Select(e => e.Name) == TenantJobAction.Approve).ToList().Count,
//                    validImpactActions.Count,
//                    "Verifying if only the configured total no. of jobs are approved and others are rejected");
//            }

//            {
//                configStore.ClearKeys();

//                const int jobCountOfEachType = 3;
//                PopulateConfigSection(configStore, jobCountOfEachType - 1);

//                // allow total jobs to be greater than the sum of each max allowed job type
//                configStore.AddKeyValue(
//                    ConfigSectionName,
//                    ParallelJobs.Constants.ConfigKeys.MaxParallelJobCountTotal,
//                    (validImpactActions.Count * jobCountOfEachType).ToString());

//                var mappedTenantJobs = CreateMappedTenantJobs(jobCountOfEachType);

//                policy.UpdateAsync(Guid.NewGuid(), new PolicyAgentDocumentForTenant(), mappedTenantJobs, new Dictionary<string, IMappedRepairTask>());

//                foreach (var impactAction in validImpactActions)
//                {
//                    int countPerJobType = mappedTenantJobs
//                        .Where(e =>
//                            e.Value.Action == TenantJobAction.Approve &&
//                            e.Value.TenantJob.ImpactDetail.ImpactAction == impactAction)
//                        .ToList()
//                        .Count;

//                    Verify.AreEqual(
//                        countPerJobType,
//                        jobCountOfEachType - 1,
//                        "Verifying if only {0} jobs of type {1} are allowed".ToString(countPerJobType, impactAction));
//                }

//                var totalActuallyAllowed =
//                    mappedTenantJobs.Where(e => e.Value.Action == TenantJobAction.Approve).ToList().Count;

//                Verify.AreEqual(
//                    totalActuallyAllowed,
//                    validImpactActions.Count * (jobCountOfEachType - 1),
//                    "Verifying if only {0} jobs are allowed even though total allowed job count is {1}".ToString(
//                        totalActuallyAllowed,
//                        validImpactActions.Count * jobCountOfEachType));
//            }

//            {
//                configStore.ClearKeys();

//                var tenantJob = TenantJobHelper.CreateNewTenantJob(ImpactActionEnum.Unknown);
//                var mappedTenantJob = new MappedTenantJob(tenantJob)
//                {
//                    Action = TenantJobAction.Approve,
//                    MapperName = "MyMapper",
//                };

//                var mappedTenantJobs = new Dictionary<string, IMappedTenantJob>(StringComparer.OrdinalIgnoreCase)
//                {
//                    { mappedTenantJob.TenantJob.GetJobId(), mappedTenantJob }
//                };

//                policy.UpdateAsync(Guid.NewGuid(), new PolicyAgentDocumentForTenant(), mappedTenantJobs, new Dictionary<string, IMappedRepairTask>());

//                Verify.AreEqual(
//                    mappedTenantJobs.Where(e => e.Value.Action == TenantJobAction.Approve).ToList().Count,
//                    0,
//                    "Verifying if Unknown impact action jobs are rejected");
//            }
//        }

//        private void PopulateConfigSection(MockConfigStore configStore, int jobCountOfEachType)
//        {
//            configStore.ClearKeys();

//            foreach (var impactAction in validImpactActions)
//            {
//                configStore.AddKeyValue(
//                    ConfigSectionName,
//                    ParallelJobs.Constants.ConfigKeys.MaxParallelJobCountKeyPrefix + impactAction,
//                    jobCountOfEachType.ToString());
//            }
//        }

//        private static IDictionary<string, IMappedTenantJob> CreateMappedTenantJobs(int jobCountOfEachType)
//        {
//            var mappedTenantJobs = new Dictionary<string, IMappedTenantJob>(StringComparer.OrdinalIgnoreCase);
//            var mappedRepairTasks = new Dictionary<string, IMappedRepairTask>(StringComparer.OrdinalIgnoreCase);

//            var tenantJobs = new List<ITenantJob>();
            
//            for (int i = 0; i < jobCountOfEachType; i++)
//            {
//                // start from 1 = exclude the Unknown impact action
//                foreach (var impactAction in validImpactActions)
//                {
//                    var tenantJob = TenantJobHelper.CreateNewTenantJob(impactAction);
//                    tenantJobs.Add(tenantJob);
//                }
//            }

//            ActionHelper.CreateMappedWorkItems(tenantJobs, new List<IRepairTask>(), mappedTenantJobs, mappedRepairTasks);
//            return mappedTenantJobs;
//        }
//    }
//}
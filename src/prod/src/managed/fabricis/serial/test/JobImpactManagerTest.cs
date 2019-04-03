// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Collections.Generic;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.WindowsAzure.ServiceRuntime.Management;
using WEX.TestExecution;

namespace System.Fabric.InfrastructureService.Test
{
    [TestClass]
    public class JobImpactManagerTest
    {
        private const string ConfigSectionName = "TestConfigSection";

        private static readonly TraceType traceType = new TraceType(typeof(JobImpactManager).Name);

        /// <summary>
        /// happy path
        /// </summary>        
        [TestMethod]
        public async Task Scenario1TestAsync()
        {
            await EvaluateImpactAsync(Guid.NewGuid().ToString(), JobType.DeploymentUpdateJob).ConfigureAwait(false);
            await EvaluateImpactAsync(null, JobType.PlatformUpdateJob).ConfigureAwait(false);
        }

        private static async Task EvaluateImpactAsync(string jobId, JobType jobType)
        {
            var instancesAndReasons1 = new[]
            {
                    "WF_IN_0", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_4", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_8", ImpactReason.ConfigurationUpdate.ToString(),
                };

            var qc = new MockQueryClient(GetNodeList());
            var jobImpactManager = new JobImpactManager(GetConfigSection(), qc);

            var notification = GetNotification(jobId, 0, instancesAndReasons1, jobType);
            var jobImpact = await jobImpactManager.EvaluateJobImpactAsync(notification).ConfigureAwait(false);
            Verify.AreEqual(jobImpact, JobImpactTranslationMode.Default, "Verifying published impact since no history is stored");

            var instancesAndReasons2 = new[]
            {
                    "WF_IN_1", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_5", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_9", ImpactReason.ConfigurationUpdate.ToString(),
                };
            notification = GetNotification(jobId, 1, instancesAndReasons2, jobType);

            // at this time, we should reuse the saved history
            jobImpact = await jobImpactManager.EvaluateJobImpactAsync(notification).ConfigureAwait(false);
            Verify.AreEqual(jobImpact, JobImpactTranslationMode.Optimized, "Verifying published impact since there wasn't a reboot of nodes previously");

            jobImpact = await jobImpactManager.EvaluateJobImpactAsync(notification).ConfigureAwait(false);
            Verify.AreEqual(jobImpact, JobImpactTranslationMode.Optimized, "Verifying invoking again (within validity time) shouldn't change result");

            // simulate nodes going down
            await Task.Delay(TimeSpan.FromSeconds(2)).ConfigureAwait(false);
            qc.Nodes = GetNodeList(DateTimeOffset.UtcNow);

            // for the next UD, since the nodes have gone down, don't use history
            var instancesAndReasons3 = new[]
            {
                    "WF_IN_2", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_6", ImpactReason.ConfigurationUpdate.ToString(),
                    "WF_IN_10", ImpactReason.ConfigurationUpdate.ToString(),
                };
            notification = GetNotification(jobId, 2, instancesAndReasons3, jobType);

            jobImpact = await jobImpactManager.EvaluateJobImpactAsync(notification).ConfigureAwait(false);
            Verify.AreEqual(jobImpact, JobImpactTranslationMode.Default, "Verifying published impact since nodes have rebooted after previous evaluation");
        }

        private static IList<INode> GetNodeList(DateTimeOffset? nodesUpTimestamp = null)
        {
            if (nodesUpTimestamp == null)
            {
                // nodes have been up an hour since now
                nodesUpTimestamp = DateTimeOffset.UtcNow - TimeSpan.FromHours(1);
            }

            var nodeList = new List<INode>();
            for (int i = 0; i < 12; i++)
            {
                nodeList.Add(new SimpleNode { NodeName = "WF." + i.ToString(), NodeUpTimestamp = nodesUpTimestamp });
            }

            return nodeList;
        }

        private static IConfigSection GetConfigSection()
        {
            var store = new MockConfigStore();
            store.AddKeyValue(ConfigSectionName, WindowsAzureInfrastructureCoordinator.ConfigEnableLearningModeForJobTypeFormatKeyName.ToString(JobType.PlatformUpdateJob), true.ToString());
            store.AddKeyValue(ConfigSectionName, WindowsAzureInfrastructureCoordinator.ConfigEnableLearningModeForJobTypeFormatKeyName.ToString(JobType.DeploymentUpdateJob), true.ToString());

            var cs = new ConfigSection(traceType, store, ConfigSectionName);

            return cs;
        }

        private static MockManagementNotificationContext GetNotification(
            string jobId, 
            int ud,             
            string[] instancesAndReasons,
            JobType jobType)
        {
            var impactedInstances = new List<ImpactedInstance>();

            if (instancesAndReasons != null)
            {
                for (int i = 0; i < instancesAndReasons.Length - 1; i += 2)
                {
                    var instance = instancesAndReasons[i];
                    var impactReason = (ImpactReason)Enum.Parse(typeof(ImpactReason), instancesAndReasons[i + 1]);

                    var impactedInstance = new ImpactedInstance(instance, new List<ImpactReason> { impactReason });
                    impactedInstances.Add(impactedInstance);
                }
            }

            var notification = new MockManagementNotificationContext
            {
                ActiveJobId = jobId ?? Guid.NewGuid().ToString(),
                ActiveJobType = jobType,
                ActiveJobStepTargetUD = ud,
                ImpactedInstances = impactedInstances,
                NotificationType = NotificationType.StartJobStep,
            };

            return notification;
        }
    }
}
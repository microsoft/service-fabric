// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using InfrastructureService.Test;
    using Microsoft.VisualStudio.TestTools.UnitTesting;
    using RD.Fabric.PolicyAgent;

    [TestClass]
    public class JobClassifierTest
    {
        private readonly MockConfigStore configStore = new MockConfigStore();
        private readonly ConfigSection configSection;
        private readonly JobClassifier jobClassifier;

        public JobClassifierTest()
        {
            configSection = new ConfigSection(Constants.TraceType, configStore, "IS");

            var env = new CoordinatorEnvironment(
                "fabric:/System/InfrastructureService",
                configSection,
                string.Empty,
                new MockInfrastructureAgentWrapper());

            jobClassifier = new JobClassifier(env);
        }

        [TestMethod]
        public void BasicTest()
        {
            var job = new MockTenantJob();

            job.JobStatus = (JobStatusEnum)999;
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Pending;
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            configStore.UpdateKey(
                configSection.Name,
                "Azure.JobClassifier.JobStatusCategory.Pending",
                "Inactive");
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Alerted;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Cancelled;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Completed;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Failed;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Suspended;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            job.JobStatus = JobStatusEnum.Executing;
            // Unknown because JobStep is null
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            var jobStep = new MockJobStepInfo();
            job.JobStep = jobStep;

            jobStep.ImpactStep = ImpactStepEnum.Default;
            // Unknown because of impact step
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            jobStep.ImpactStep = (ImpactStepEnum)999;
            // Unknown because of impact step
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            jobStep.ImpactStep = ImpactStepEnum.ImpactStart;

            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement;
            Assert.AreEqual(JobPhase.ImpactStartWaitingForAck, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Acknowledged;
            Assert.AreEqual(JobPhase.ImpactStartAcked, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Timedout;
            Assert.AreEqual(JobPhase.ImpactStartAcked, jobClassifier.GetJobPhase(job));

            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Alerted;
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            configStore.UpdateKey(
                configSection.Name,
                "Azure.JobClassifier.ImpactStartAcknowledgementStatusAlerted",
                "Inactive");
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            jobStep.AcknowledgementStatus = (AcknowledgementStatusEnum)999;
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            jobStep.ImpactStep = ImpactStepEnum.ImpactEnd;

            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement;
            Assert.AreEqual(JobPhase.ImpactEndWaitingForAck, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Acknowledged;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Timedout;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.Alerted;
            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));
            jobStep.AcknowledgementStatus = (AcknowledgementStatusEnum)999;
            Assert.AreEqual(JobPhase.Unknown, jobClassifier.GetJobPhase(job));

            // Compare versions

            configStore.ClearKeys();

            job.JobStatus = JobStatusEnum.Failed;
            jobStep.ImpactStep = ImpactStepEnum.ImpactEnd;
            jobStep.AcknowledgementStatus = AcknowledgementStatusEnum.WaitingForAcknowledgement;

            Assert.AreEqual(JobPhase.Inactive, jobClassifier.GetJobPhase(job));

            configStore.UpdateKey(
                configSection.Name,
                "Azure.JobClassifier.Version",
                "1");

            Assert.AreEqual(JobPhase.ImpactEndWaitingForAck, jobClassifier.GetJobPhase(job));
        }
    }
}
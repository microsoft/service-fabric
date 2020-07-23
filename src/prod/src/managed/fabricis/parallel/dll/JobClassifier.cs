// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using RD.Fabric.PolicyAgent;

namespace System.Fabric.InfrastructureService.Parallel
{
    internal enum JobCategory
    {
        Normal,
        VendorRepairBegin,
        VendorRepairEnd,
    }

    internal enum JobPhase
    {
        /// <summary>
        /// The job phase cannot be determined.
        /// </summary>
        Unknown,

        /// <summary>
        /// The job is neither executing nor waiting for tenant ack.
        /// </summary>
        Inactive,

        /// <summary>
        /// The job is waiting for an ack prior to starting the work.
        /// </summary>
        ImpactStartWaitingForAck,

        /// <summary>
        /// The job is executing.
        /// </summary>
        ImpactStartAcked,

        /// <summary>
        /// The job has completed and is waiting for a final ack.
        /// </summary>
        ImpactEndWaitingForAck,
    }

    /// <summary>
    /// Classifies jobs into a <see cref="JobCategory"/> and <see cref="JobPhase"/>
    /// for use by the <see cref="Reconciler"/>.
    /// </summary>
    internal sealed class JobClassifier
    {
        private enum JobStatusCategory
        {
            Unknown,
            Active,
            Inactive,
        }

        private readonly IConfigSection config;
        private readonly TraceType traceType;

        public JobClassifier(CoordinatorEnvironment environment)
        {
            this.config = environment.Config;
            this.traceType = environment.CreateTraceType("JobClassifier");
        }

        public JobCategory GetJobCategory(ITenantJob job)
        {
            if (job.IsVendorRepairBegin())
                return JobCategory.VendorRepairBegin;

            if (job.IsVendorRepairEnd())
                return JobCategory.VendorRepairEnd;

            return JobCategory.Normal;
        }

        public JobPhase GetJobPhase(ITenantJob job)
        {
            // Versioned to allow fallback to previous job
            // classification logic in case live site issues
            // require it.
            int version = this.config.ReadConfigValue(
                Constants.ConfigKeys.JobClassifierVersion,
                defaultValue: 2);

            switch (version)
            {
                case 2:
                    return GetJobPhase_V2(job);
                default:
                    return GetJobPhase_V1(job);
            }
        }

        private static JobPhase GetJobPhase_V1(ITenantJob job)
        {
            if (job.IsWaitingForImpactStartAck())
                return JobPhase.ImpactStartWaitingForAck;

            if (job.IsImpactStartAcked())
                return JobPhase.ImpactStartAcked;

            if (job.IsWaitingForImpactEndAck())
                return JobPhase.ImpactEndWaitingForAck;

            return JobPhase.Inactive;
        }

        private JobPhase GetJobPhase_V2(ITenantJob job)
        {
            // First classify by top-level job status. Only active jobs
            // require further classification.
            JobStatusCategory jobStatusCategory = this.config.ReadConfigValue(
                Constants.ConfigKeys.JobClassifierJobStatusCategoryFormat.ToString(job.JobStatus),
                GetDefaultJobStatusCategory(job.JobStatus));

            switch (jobStatusCategory)
            {
                case JobStatusCategory.Inactive:
                    return JobPhase.Inactive;

                case JobStatusCategory.Active:
                    return GetJobPhaseForActiveStep(job.JobStep);

                case JobStatusCategory.Unknown:
                default:
                    return JobPhase.Unknown;
            }
        }

        private static JobStatusCategory GetDefaultJobStatusCategory(JobStatusEnum jobStatus)
        {
            switch (jobStatus)
            {
                case JobStatusEnum.Executing:
                    return JobStatusCategory.Active;

                case JobStatusEnum.Alerted:
                case JobStatusEnum.Cancelled:
                case JobStatusEnum.Completed:
                case JobStatusEnum.Failed:
                case JobStatusEnum.Suspended:
                    return JobStatusCategory.Inactive;

                case JobStatusEnum.Pending:
                default:
                    return JobStatusCategory.Unknown;
            }
        }

        private JobPhase GetJobPhaseForActiveStep(IJobStepInfo jobStep)
        {
            if (jobStep == null)
            {
                this.traceType.WriteInfo("Job phase is unknown because jobStep is null");
                return JobPhase.Unknown;
            }

            switch (jobStep.ImpactStep)
            {
                case ImpactStepEnum.ImpactStart:
                    return GetJobPhaseForImpactStartStep(jobStep);

                case ImpactStepEnum.ImpactEnd:
                    return GetJobPhaseForImpactEndStep(jobStep);

                default:
                    this.traceType.WriteInfo(
                        "Job phase is unknown because impact step is {0}",
                        jobStep.ImpactStep);
                    return JobPhase.Unknown;
            }
        }

        private JobPhase GetJobPhaseForImpactStartStep(IJobStepInfo jobStep)
        {
            switch (jobStep.AcknowledgementStatus)
            {
                case AcknowledgementStatusEnum.WaitingForAcknowledgement:
                    return JobPhase.ImpactStartWaitingForAck;

                case AcknowledgementStatusEnum.Acknowledged:
                case AcknowledgementStatusEnum.Timedout:
                    return JobPhase.ImpactStartAcked;

                case AcknowledgementStatusEnum.Alerted:
                    // Configurable because it may be desirable to remap this
                    // to JobPhase.Inactive in case jobs never change from
                    // Executing/ImpactStart/Alerted to Alerted/-/-.
                    return this.config.ReadConfigValue(
                        Constants.ConfigKeys.JobClassifierImpactStartAcknowledgementStatusAlerted,
                        JobPhase.Unknown);

                default:
                    this.traceType.WriteInfo(
                        "Job phase is unknown because {0} ack status is {1}",
                        jobStep.ImpactStep,
                        jobStep.AcknowledgementStatus);
                    return JobPhase.Unknown;
            }
        }

        private JobPhase GetJobPhaseForImpactEndStep(IJobStepInfo jobStep)
        {
            switch (jobStep.AcknowledgementStatus)
            {
                case AcknowledgementStatusEnum.WaitingForAcknowledgement:
                    return JobPhase.ImpactEndWaitingForAck;

                case AcknowledgementStatusEnum.Acknowledged:
                case AcknowledgementStatusEnum.Timedout:
                case AcknowledgementStatusEnum.Alerted:
                    return JobPhase.Inactive;

                default:
                    this.traceType.WriteInfo(
                        "Job phase is unknown because {0} ack status is {1}",
                        jobStep.ImpactStep,
                        jobStep.AcknowledgementStatus);
                    return JobPhase.Unknown;
            }
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Encapsulate information on scheduling of an analysis
    /// </summary>
    [DataContract]
    [Serializable]
    internal sealed class AnalysisSchedulingInformation
    {
        private const uint MaxAnalysisAttempts = 3;

        private const uint DefaultAnalysisAttempts = 2;

        [DataMember(IsRequired = true)] private int instanceAnalysisAttemptedCount;

        [DataMember(IsRequired = true)] private uint maxAttempts;

        [DataMember(IsRequired = true)] private AnalysisStatus currentStatus;

        [DataMember(IsRequired = true)] private DateTime nextActivationTime;

        [DataMember(IsRequired = true)] private Continuation continuation;

        [DataMember(IsRequired = true)] private DateTime analysisStartTime;

        [DataMember(IsRequired = true)] private DateTime analysisEndTime;

        private bool isBeingUsed;

        /// <summary>
        /// Create an instance of <see cref="AnalysisSchedulingInformation"/>
        /// </summary>
        /// <param name="maxAnalysisAttempts"></param>
        public AnalysisSchedulingInformation(uint maxAnalysisAttempts = DefaultAnalysisAttempts)
        {
            if (maxAnalysisAttempts <= 0 || maxAnalysisAttempts > MaxAnalysisAttempts)
            {
                throw new ArgumentOutOfRangeException(
                    "maxAnalysisAttempts",
                    string.Format(CultureInfo.InvariantCulture, "Maximum Analysis attempt count is : '{0}'", MaxAnalysisAttempts));
            }

            this.maxAttempts = maxAnalysisAttempts;
            this.instanceAnalysisAttemptedCount = 0;
            this.currentStatus = AnalysisStatus.NotStarted;
            this.continuation = null;
        }

        /// <summary>
        /// Current Status of this analysis processing.
        /// </summary>
        public AnalysisStatus CurrentStatus
        {
            get { return this.currentStatus; }
        }

        /// <summary>
        /// Time Analysis Started
        /// </summary>
        public DateTime AnalysisStartTime
        {
            get { return this.analysisStartTime; }
        }

        /// <summary>
        /// Time Analysis Finished.
        /// </summary>
        public DateTimeOffset AnalysisEndTime
        {
            get { return this.analysisEndTime; }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Current Status {0}, AttemptedCount {1}, MaxAttempts {2}, Next Activation Count {3}",
                this.currentStatus,
                this.instanceAnalysisAttemptedCount,
                this.maxAttempts,
                this.GetNextActivationTimeUtc().GetValueOrDefault(DateTime.MinValue));
        }

        internal void StartUse()
        {
            Assert.IsFalse(this.isBeingUsed == true, "this.isBeingUsed == true");
            this.isBeingUsed = true;
        }

        internal void StopUse()
        {
            Assert.IsFalse(this.isBeingUsed == false, "this.isBeingUsed == false");
            this.isBeingUsed = false;
        }

        internal bool InUse()
        {
            return this.isBeingUsed;
        }

        internal void SetContinuation(Continuation continueBehavior)
        {
            Assert.IsNotNull(continueBehavior);
            this.continuation = continueBehavior;
            if (this.continuation.Equals(Continuation.Done))
            {
                this.currentStatus = AnalysisStatus.Completed;
                return;
            }

            switch (this.continuation.ContinuationType)
            {
                case ContinuationType.WaitForTime:
                    Assert.IsTrue(this.continuation.SuspendTimeout.HasValue, "Suspend Timeout should have value");
                    this.currentStatus = AnalysisStatus.Suspended;
                    this.nextActivationTime = DateTime.UtcNow.Add(this.continuation.SuspendTimeout.Value);
                    break;

                case ContinuationType.WaitForFabricEvent:
                    this.currentStatus = AnalysisStatus.Suspended;
                    break;

                case ContinuationType.WaitForInterest:
                    this.currentStatus = AnalysisStatus.Suspended;
                    break;

                default:
                    throw new NotSupportedException(string.Format(
                        CultureInfo.InvariantCulture,
                        "Continuation Type: {0} is Unsupported",
                        this.continuation.ContinuationType));
            }
        }

        /// <summary>
        /// Calculate approximately when a workflow with this continuation will be activated 
        /// </summary>
        /// <returns>Approximate UTC time of workflow next activation or null if the continuation does not specify a suspend timeout</returns>
        internal DateTime? GetNextActivationTimeUtc()
        {
            if (this.currentStatus != AnalysisStatus.Suspended)
            {
                return null;
            }

            return this.nextActivationTime;
        }

        internal TraceRecord GetEventBeingWaitedOn()
        {
            Assert.IsTrue(this.currentStatus == AnalysisStatus.Suspended, string.Format(CultureInfo.InvariantCulture, "This routine not valid when Current status: {0}", this.currentStatus));
            if (this.continuation.ContinuationType != ContinuationType.WaitForFabricEvent)
            {
                return null;
            }

            return this.continuation.TraceRecord;
        }

        internal ContinuationType GetContinuationType()
        {
            if (this.continuation != null)
            {
                return this.continuation.ContinuationType;
            }

            return ContinuationType.Invalid;
        }

        internal void IncrementAnalysisAttemptedCount()
        {
            Assert.IsFalse(this.IsOutOfAttempts(), "This schedule is already out of attempts");
            ++this.instanceAnalysisAttemptedCount;
        }

        internal bool IsOutOfAttempts()
        {
            return this.instanceAnalysisAttemptedCount >= this.maxAttempts;
        }

        /// <summary>
        /// Mark analysis as finished.
        /// </summary>
        /// <remarks>
        /// Only NotStarted -> Queued or Suspended -> Queued are supported transitions.
        /// </remarks>
        internal void MarkQueued()
        {
            Assert.IsTrue(
                this.currentStatus == AnalysisStatus.NotStarted || this.currentStatus == AnalysisStatus.Suspended,
                () => string.Format(CultureInfo.InvariantCulture, "Current Status {0} -> Queued Not valid", this.currentStatus));
            this.currentStatus = AnalysisStatus.Queued;
        }

        /// <summary>
        /// Mark analysis as finished.
        /// </summary>
        /// <remarks>
        /// Only InProgress -> Finished is supported.
        /// </remarks>
        internal void MarkFinished()
        {
            Assert.IsTrue(this.currentStatus == AnalysisStatus.InProgress, () => string.Format(CultureInfo.InvariantCulture, "Current Status {0} -> Finished Not valid", this.currentStatus));
            this.currentStatus = AnalysisStatus.Completed;
            this.analysisEndTime = DateTime.UtcNow.ToUniversalTime();
        }

        /// <summary>
        /// Mark status as started
        /// </summary>
        /// <remarks>
        /// Only NotStarted -> Started or Queued -> Started are supported.
        /// TODO:: Fix Suspended???
        /// </remarks>
        internal void MarkStarted()
        {
            Assert.IsFalse(this.IsOutOfAttempts(), "Already out of Attempts. Can't be transitioned to started");
            Assert.IsTrue(
                this.currentStatus == AnalysisStatus.NotStarted || this.currentStatus == AnalysisStatus.Queued || this.currentStatus == AnalysisStatus.Suspended,
                string.Format(CultureInfo.InvariantCulture, "Transition From Current Status: {0} -> Started Not valid", this.currentStatus));

            this.currentStatus = AnalysisStatus.InProgress;

            if (this.currentStatus == AnalysisStatus.NotStarted)
            {
                this.analysisStartTime = DateTime.UtcNow.ToUniversalTime();
            }
        }

        internal void MarkFailed()
        {
            Assert.IsTrue(
                this.currentStatus == AnalysisStatus.InProgress,
                () => string.Format(CultureInfo.InvariantCulture, "Transition From Current Status: {0} -> Failed Not valid", this.currentStatus));

            this.currentStatus = AnalysisStatus.Failed;

            this.analysisEndTime = DateTime.UtcNow.ToUniversalTime();
        }
    }
}
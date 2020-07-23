// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterAnalysis.AnalysisCore.Callback;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    public enum ContinuationType
    {
        /// <summary>
        /// This indicate that continuation is based on waiting for a specific time interval.
        /// </summary>
        WaitForTime,

        /// <summary>
        /// This indicates that continuation is based on waiting for a specific fabric event.
        /// </summary>
        WaitForFabricEvent,

        /// <summary>
        /// This indicates that continuation is based on waiting for a specific scenario to occur.
        /// </summary>
        WaitForScenario,

        /// <summary>
        /// The containers can register for interesting Events and this indicates that
        /// continuation is based on waiting for one of the registered interest to occur.
        /// </summary>
        WaitForInterest,

        /// <summary>
        /// Not defined yet
        /// </summary>
        Invalid
    }

    /// <summary>
    /// Represents the resume behavior requested by a workflow after it finished its current activity by exiting the workflow entry point
    /// </summary>
    [DataContract]
#pragma warning disable CS0659 // Type overrides Object.Equals(object o) but does not override Object.GetHashCode()
    public sealed class Continuation
#pragma warning restore CS0659 // Type overrides Object.Equals(object o) but does not override Object.GetHashCode()
    {
        private static readonly Continuation DefaultContinuation = ResumeAfter(TimeSpan.Zero);

        private static readonly Continuation ResumeAfter1Ms = ResumeAfter(TimeSpan.FromMilliseconds(1));

        private Continuation(TimeSpan? suspendTimeout)
        {
            this.SuspendTimeout = suspendTimeout;
            this.ContinuationType = ContinuationType.WaitForTime;
        }

        private Continuation(TraceRecord traceRecord)
        {
            this.TraceRecord = traceRecord;
            this.ContinuationType = ContinuationType.WaitForFabricEvent;
        }

        private Continuation(Scenario scenario)
        {
            this.Scenario = scenario;
            this.ContinuationType = ContinuationType.WaitForScenario;
        }

        private Continuation(ContinuationType type)
        {
            this.ContinuationType = type;
        }

        /// <summary>
        /// Gets the default continuation which is to indicate that agent is done.
        /// </summary>
        public static Continuation Done
        {
            get { return DefaultContinuation; }
        }

        public static Continuation WaitForInterest
        {
            get { return new Continuation(ContinuationType.WaitForInterest); }
        }

        /// <summary>
        /// Gets the default continuation which is to indicate that agent should be resumed at first possible opportunity
        /// </summary>
        public static Continuation ResumeImmediately
        {
            get { return ResumeAfter1Ms; }
        }

        /// <summary>
        /// Type of continuation.
        /// </summary>
        [DataMember(IsRequired = true)]
        public ContinuationType ContinuationType { get; private set; }

        /// <summary>
        /// Get the Scenario the workload needs to wait to happen to resume.
        /// </summary>
        [DataMember(IsRequired = true)]
        public Scenario Scenario { get; private set; }

        /// <summary>
        /// Get the fabric event the workflow needs to wait to happen to resume.
        /// </summary>
        [DataMember(IsRequired = true)]
        public TraceRecord TraceRecord { get; private set; }

        /// <summary>
        /// Gets the amount of time the workflow has requested to stay suspended before the runtime resumes it.
        /// </summary>
        [DataMember(IsRequired = true)]
        public TimeSpan? SuspendTimeout { get; private set; }

        /// <summary>
        /// Creates a continuation which instructs the runtime to wait for the specified time and resume the workflow
        /// </summary>
        /// <param name="suspendTimeout">The amount of time the runtime should wait before resuming the workflow</param>
        /// <returns>New continuation with "sleep" semantics</returns>
        public static Continuation ResumeAfter(TimeSpan suspendTimeout)
        {
            return new Continuation(suspendTimeout);
        }

        /// <summary>
        /// Create a continuation that signals the runtime to resume the workflow when the specific
        /// fabric event is seen.
        /// </summary>
        /// <param name="traceRecord">Fabric event to wait on</param>
        /// <returns></returns>
        public static Continuation ResumeOnFabricEvent(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord);
            return new Continuation(traceRecord);
        }

        /// <summary>
        /// Create a continuation that signals the runtime to resume the workflow when the specific
        /// Scenario is hit
        /// </summary>
        /// <param name="scenario">Scenario to wait on</param>
        /// <returns></returns>
        public static Continuation ResumeOnScenario(Scenario scenario)
        {
            return new Continuation(scenario);
        }

        /// <inheritdoc />
        public override bool Equals(object obj)
        {
            if (obj == null)
            {
                return false;
            }

            var otherObj = obj as Continuation;
            if (otherObj == null)
            {
                return false;
            }

            if (this.ContinuationType != otherObj.ContinuationType)
            {
                return false;
            }

            if (this.ContinuationType == ContinuationType.WaitForTime)
            {
                return this.SuspendTimeout.Value == otherObj.SuspendTimeout.Value;
            }

            if (this.ContinuationType == ContinuationType.WaitForFabricEvent)
            {
                return this.TraceRecord == otherObj.TraceRecord;
            }

            return true;
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Type: {0}, SuspendTimeout: {1}",
                this.ContinuationType,
                this.SuspendTimeout.HasValue ? this.SuspendTimeout.ToString() : "N/A");
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;
    using ClusterAnalysis.AnalysisCore.AnalysisEvents;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.Fix;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.AnalysisCore.RootCause;
    using Common;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Container for an analysis
    /// </summary>
    [DataContract]
    [KnownType(typeof(FixContainer))]
    [KnownType(typeof(RootCauseContainer))]
    [KnownType(typeof(ScenarioData))]
    [KnownType(typeof(InterestFilter))]
    [KnownType(typeof(FabricAnalysisEvent))]
    internal sealed class AnalysisContainer : IHasUniqueIdentity, IComparable<AnalysisContainer>
    {
        private const uint MaxAnalysisAttempts = 3;

        private const int DefaultAnalysisAttempts = 2;

        [DataMember(IsRequired = true)]
        private StringBuilder analysisOutput;

        [DataMember(IsRequired = true)]
        private RootCauseContainer rootCauseContainer;

        [DataMember(IsRequired = true)]
        private FixContainer fixContainer;

        [DataMember(IsRequired = true)]
        private List<TraceRecord> relevantTraces;

        [DataMember(IsRequired = true)]
        private IDictionary<string, string> customContext;

        [DataMember(IsRequired = true)]
        private IList<string> exceptionEncountered;

        [DataMember(IsRequired = true)]
        private int maxAttempts;

        [DataMember(IsRequired = true)]
        private AnalysisStatus currentStatus;

        [DataMember(IsRequired = true)]
        private ProgressTracker progressTracker;

        [DataMember(IsRequired = true)]
        private InterestFilter interestFilter;

        public AnalysisContainer(FabricAnalysisEvent analysisEvent, AgentIdentifier agentIdentifier, int maxAnalysisAttempts = DefaultAnalysisAttempts)
        {
            if (maxAnalysisAttempts <= 0 || maxAnalysisAttempts > MaxAnalysisAttempts)
            {
                throw new ArgumentOutOfRangeException(
                    "maxAnalysisAttempts",
                    string.Format(CultureInfo.InvariantCulture, "Maximum Analysis attempt count is : '{0}'", MaxAnalysisAttempts));
            }

            Assert.IsNotNull(analysisEvent, "analysisEvent != null");
            this.AnalysisEvent = analysisEvent;
            this.maxAttempts = maxAnalysisAttempts;
            this.analysisOutput = new StringBuilder();
            this.rootCauseContainer = new RootCauseContainer();
            this.fixContainer = new FixContainer();
            this.relevantTraces = new List<TraceRecord>();
            this.customContext = new Dictionary<string, string>();
            this.Agent = agentIdentifier;
        }

        /// <summary>
        /// The identifier of the Agent which is responsible for creating and populating this analysis container
        /// </summary>
        [DataMember(IsRequired = true)]
        public AgentIdentifier Agent { get; private set; }

        /// <summary>
        /// The analysis event associated with this Analysis
        /// </summary>
        [DataMember(IsRequired = true)]
        public FabricAnalysisEvent AnalysisEvent { get; private set; }

        /// <summary>
        /// The scenario data that triggered this Analysis.
        /// If an analysis get resumed due again due to another data point
        /// this field will point to the most recent data that triggered it.
        /// </summary>
        [DataMember(IsRequired = true)]
        internal ScenarioData CurrentInvokationData { get; set; }

        /// <summary>
        /// Gets or sets interest filter. This encapsulate information on future events this object is interested in.
        /// </summary>
        /// <returns></returns>
        internal InterestFilter InterestFilter
        {
            get { return this.interestFilter; }
            set { this.interestFilter = value; }
        }

        /// <summary>
        ///  Get a collection of Exceptions encountered during this analysis
        /// </summary>
        /// <returns></returns>
        public ReadOnlyCollection<string> GetExceptionSeen()
        {
            return new ReadOnlyCollection<string>(this.exceptionEncountered);
        }

        /// <summary>
        /// Gets the current status of this Analysis
        /// </summary>
        /// <returns></returns>
        public AnalysisStatus GetCurrentStatus()
        {
            return this.currentStatus;
        }

        /// <summary>
        /// Get the Unique ID identifying this instance.
        /// </summary>
        /// <returns></returns>
        public Guid GetUniqueIdentity()
        {
            return this.AnalysisEvent.InstanceId;
        }

        #region Public_Abstractions

        /// <summary>
        /// Get fixes
        /// </summary>
        /// <returns></returns>
        public FixContainer GetFix()
        {
            return this.fixContainer;
        }

        /// <summary>
        /// Get root causes.
        /// </summary>
        /// <returns></returns>
        public RootCauseContainer GetRootCause()
        {
            return this.rootCauseContainer;
        }

        /// <summary>
        /// Get a friendly output of analysis.
        /// </summary>
        /// <returns></returns>
        public string GetAnalysisOutput()
        {
            return this.analysisOutput.ToString();
        }

        /// <summary>
        /// Get custom context associated with current analysis
        /// </summary>
        /// <returns></returns>
        public IReadOnlyDictionary<string, string> GetCustomContext()
        {
            return new ReadOnlyDictionary<string, string>(this.customContext);
        }

        public int CompareTo(AnalysisContainer other)
        {
            if (other == null)
            {
                throw new ArgumentNullException("other");
            }

            return this.AnalysisEvent.OriginTime.CompareTo(other.AnalysisEvent.OriginTime);
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Current Status: {0}, CurrentInvokationData: {1}, Analysis: {2}",
                this.currentStatus,
                this.CurrentInvokationData,
                this.AnalysisEvent);
        }

        #endregion Public_Abstractions

        #region Internal_Abstractions

        internal void SetAnalysisStatus(AnalysisStatus status)
        {
            this.currentStatus = status;
        }

        internal void SetProgressedTill(ProgressTracker currentCheckpoint)
        {
            this.progressTracker = currentCheckpoint;
        }

        internal ProgressTracker GetProgressedTill()
        {
            return this.progressTracker;
        }

        internal void AddExceptionSeen(Exception exp)
        {
            Assert.IsNotNull(exp);
            if (this.exceptionEncountered == null)
            {
                this.exceptionEncountered = new List<string>();
            }
        }

        /// <summary>
        /// Add analysis to analysis output.
        /// </summary>
        /// <param name="analysisMsg"></param>
        internal void AddToAnalysisOutput(string analysisMsg)
        {
            this.analysisOutput.AppendLine().Append(string.Format(CultureInfo.InvariantCulture, "Stage: {0}, Msg: {1}", this.GetProgressedTill(), analysisMsg));
        }

        /// <summary>
        /// Add a fix to the analysis
        /// </summary>
        /// <param name="fix"></param>
        internal void AddFix(IFix fix)
        {
            Assert.IsNotNull(fix, "Fix can't be null");
            this.fixContainer.Add(fix);
        }

        /// <summary>
        /// Add a root cause to the analysis
        /// </summary>
        /// <param name="rootCause"></param>
        internal void AddRootCause(IRootCause rootCause)
        {
            Assert.IsNotNull(rootCause, "Root cause can't be null");
            this.rootCauseContainer.Add(rootCause);
        }

        internal void AddCustomContext(string name, string value)
        {
            Assert.IsNotEmptyOrNull(name, "name");
            Assert.IsNotEmptyOrNull(value, "value");
            this.customContext[name] = value;
        }

        #endregion Internal_Abstractions
    }
}
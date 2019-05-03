// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System.Collections.Generic;
    using System.Fabric.Query;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.AnalysisCore.Insights.Agents;
    using ClusterAnalysis.Common.Runtime;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Generates Insight for a partition.
    /// </summary>
    internal class PartitionInsightGenerator : BaseInsightGenerator
    {
        private AgentIdentifier primaryMoveAnalysisAgentIdentifier;

        internal PartitionInsightGenerator(IInsightRuntime runtime, ICallbackStore callbackStore, CancellationToken token) : base(runtime, callbackStore, token)
        {
            Assert.IsNotNull(runtime, "runtime != null");

            this.primaryMoveAnalysisAgentIdentifier = AgentIdentifier.PrimaryMoveAnalysisAgent;
        }

        /// <inheritdoc />
        public override InsightType GetInsightType()
        {
            return InsightType.PartitionInsight;
        }

        protected override bool IsExcludedFromObservation(BaseEntity entity)
        {
            Assert.IsNotNull(entity);

            var partitionEntity = entity as PartitionEntity;
            Assert.IsNotNull(
                partitionEntity,
                string.Format(CultureInfo.InvariantCulture, "Received Type '{0}'. Expected Type: PartitionEntity", entity.GetType()));

            // If partition is being deleted, we don't try to analyze it or generate insight.
            if (partitionEntity.PartitionStatus == ServicePartitionStatus.Deleting)
            {
                this.Logger.LogMessage("IsExcludedFromObservation:: Extracted Partition: '{0}' is being Deleted. Skipp analyzing it.", partitionEntity);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        protected override Task InternalHandleIncomingEventNotificationAsync(ScenarioData scenarioData, BaseEntity baseEntity)
        {
            return Task.FromResult(true);
        }

        /// <inheritdoc />
        protected override IList<AgentIdentifier> GetSignalTriggeredAgents()
        {
            return new List<AgentIdentifier> { this.primaryMoveAnalysisAgentIdentifier };
        }
    }
}
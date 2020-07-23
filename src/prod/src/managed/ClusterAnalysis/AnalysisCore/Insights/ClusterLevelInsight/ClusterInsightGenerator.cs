// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.ClusterLevelInsight
{
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.AnalysisCore.Callback;
    using ClusterAnalysis.AnalysisCore.ClusterEntityDescription;
    using ClusterAnalysis.Common.Runtime;

    internal class ClusterInsightGenerator : BaseInsightGenerator
    {
        /// <inheritdoc />
        internal ClusterInsightGenerator(IInsightRuntime runtime, ICallbackStore callbackStore, CancellationToken token) : base(runtime, callbackStore, token)
        {
            this.Logger.LogMessage("ClusterInsightGenerator:: Creating Object");
        }

        /// <inheritdoc />
        public override InsightType GetInsightType()
        {
            return InsightType.ClusterInsight;
        }

        /// <inheritdoc />
        protected override Task InternalHandleIncomingEventNotificationAsync(ScenarioData scenarioData, BaseEntity entity)
        {
            this.Logger.LogMessage("InternalHandleIncomingEventNotificationAsync:: Received Signal '{0}'", scenarioData.TraceRecord);
            return Task.FromResult(true);
        }
    }
}
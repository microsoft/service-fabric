// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisAgentConfig.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Globalization;
    using ClusterAnalysis.Common.Config;

    internal sealed class PrimaryMoveAnalysisAgentConfig : INamedConfig
    {
        /// <inheritdoc />
        public string Name
        {
            get { return "PrimaryMoveAnalysisAgentConfig"; }
        }

        /// <summary>
        /// Gets or sets the minimum reconfiguration duration for an analysis to Kick in.
        /// </summary>
        public TimeSpan ReconfigTimeThresholdForAnalysis { get; internal set; }

        /// <summary>
        /// Gets or sets if we need to analyze when a reconfiguration fails
        /// </summary>
        public bool ShouldAnalyzeFailedReconfigurations { get; internal set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Name: {0}, ReconfigTimeThresholdForAnalysis: {1}, ShouldAnalyzeFailedReconfigurations: {2}",
                this.Name,
                this.ReconfigTimeThresholdForAnalysis,
                this.ShouldAnalyzeFailedReconfigurations);
        }
    }
}
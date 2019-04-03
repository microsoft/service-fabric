// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis
{
    internal enum FeatureState
    {
        /// <summary>
        /// The feature state is Not available.
        /// </summary>
        NotAvailable,

        /// <summary>
        /// Currently Running
        /// </summary>
        Running,

        /// <summary>
        /// Currently Stopped
        /// </summary>
        Stopped,

        /// <summary>
        /// Failed State
        /// </summary>
        Failed
    }

    internal class AnalysisFeatureStatus
    {
        public static readonly AnalysisFeatureStatus Default =
            new AnalysisFeatureStatus { CurrentState = FeatureState.NotAvailable, UserDesiredState = FeatureState.NotAvailable };

        /// <summary>
        /// Tracks the current status of the Feature
        /// </summary>
        public FeatureState CurrentState { get; set; }

        /// <summary>
        /// Tracks the Desired state of the Feature
        /// </summary>
        public FeatureState UserDesiredState { get; set; }
    }
}
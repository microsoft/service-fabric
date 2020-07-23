// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using ClusterAnalysis.Common.Store;

    internal static class AgentConstants
    {
        // DON'T CHANGE - BAD THINGS WILL HAPPEN IF YOU DO.
        public const string AnalysisUnprocessedSignalStoreId = "AnalysisUnprocessedSignalStore-cacef994-3436-45ab-961e-1b179118142e";
        public const string AnalysisMetadataStoreId = "AnalysisMetadataStore-da395fd6-e450-4c03-b500-4a302ba2fc52";
        public const string AnalysisContainerStoreId = "AnalysisContainerStore-5fbfdd75-2efc-4901-8b08-b97b85375c2e";
        public static readonly DataRetentionPolicy AnalysisContainerStoreRetentionPolicy = AgeBasedRetentionPolicy.OneDay;
        public static readonly DataRetentionPolicy AnalysisMetadataStoreRetentionPolicy = AgeBasedRetentionPolicy.OneDay;
        public static readonly DataRetentionPolicy AnalysisUnprocessedSignalStoreRetentionPolicy = AgeBasedRetentionPolicy.OneDay;
    }
}

// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveReason.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    public enum PrimaryMoveReason
    {
        Unknown = 0,
        NodeDown = 1,
        ApplicationHostDown = 2,
        ClientApiReportFault = 3,
        ServiceApiReportFault = 4,
        Upgrade = 5,
        ClientPromoteToPrimaryApiCall = 6,
        ClientApiMovePrimary = 7,
        LoadBalancing = 8,
        ConstraintViolation = 9,
        MakeRoomForNewReplicas = 10,
        Failover = 10,
        SwapPrimary = 11
    }
}
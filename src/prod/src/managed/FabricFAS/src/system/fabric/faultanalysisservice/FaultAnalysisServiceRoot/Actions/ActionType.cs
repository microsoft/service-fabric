// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Actions
{   
    internal enum ActionType
    {
        InvokeDataLoss,
        InvokeQuorumLoss,
        RestartPartition,
        TestStuck, // test only
        TestRetryStep, // test only
        StartNode,
        StopNode,
    }
}
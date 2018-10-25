// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Chaos
{
    internal enum ChaosStopReason : byte
    {
        ClientApi = 0,
        TimeToRunOver = 1
    }
}
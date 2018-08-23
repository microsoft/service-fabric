// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.Common
{
    [Flags]
    internal enum TestMode
    {
        Invalid = 0x0000,
        ChaosTestMode = 0x0001
    }
}
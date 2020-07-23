// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum DurabilityLevel
    {
        Bronze, // No MR
        Silver, // MR time out is 30 min
        Gold, // The MR time out is 2 hours
        Platimun // MR time out is infinite
    }
}
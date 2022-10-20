// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    internal enum DiagnosticStoreType : int
    {
        None = 0,
        FileShare = 1,
        AzureStorage = 2
    }
}
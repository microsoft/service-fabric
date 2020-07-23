// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    internal static class AzureBlobEtwConstants
    {
        internal const int MethodExecutionMaxRetryCount = 3;
        internal const int MethodExecutionInitialRetryIntervalMs = 1000;
        internal const int MethodExecutionMaxRetryIntervalMs = int.MaxValue;
    }
}
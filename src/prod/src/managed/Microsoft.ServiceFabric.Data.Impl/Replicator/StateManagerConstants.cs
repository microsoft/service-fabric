// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    internal static class StateManagerConstants
    {
        public const int GuidSizeInBytes = 16;

        public const int InvalidTransactionId = -1;

        public const long InvalidLSN = -1;

        public const long ZeroLSN = 0;

        /// <summary>
        /// Max time a RemoveUnreferencedStateProviders should take.
        /// </summary>
        public const long MaxRemoveUnreferencedStateProvidersTime = 1000 * 30;
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class Constants
        {
        public:
            static FABRIC_STATE_PROVIDER_ID const EmptyStateProviderId = 0L;
            static FABRIC_STATE_PROVIDER_ID const InvalidStateProviderId = 0L;

            static LONG64 const InvalidTransactionId = -1L;
            
            // Temporary until Concurrent Dictionary becomes auto re-partitioning.
            static ULONG const NumberOfBuckets = 1031;

            static LONG const CurrentVersion = 1;

            static ULONG const KHashTableSaturationUpperLimit = 60;
            static ULONG const KHashTableSaturationLowerLimit = 5;

            /// <summary>
            /// Max time a RemoveUnreferencedStateProviders should take.
            /// </summary>
            static LONG64 const MaxRemoveUnreferencedStateProvidersTime = 1000 * 30;
        };
    }
}

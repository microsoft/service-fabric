// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace ReplicatedStoreTransactionReplicatorThrottleState
    {
        enum Enum
        {
            Uninitialized = 0,
            Initialized = 1,
            Started = 2,
            Stopped = 3,

            FirstValidEnum = Uninitialized,
            LastValidEnum = Stopped
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( ReplicatedStoreTransactionReplicatorThrottleState )
    };
}


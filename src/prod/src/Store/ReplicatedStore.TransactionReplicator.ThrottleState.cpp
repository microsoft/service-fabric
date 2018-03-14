// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace ReplicatedStoreTransactionReplicatorThrottleState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Uninitialized: w << "Uninitialized"; return;
                case Initialized: w << "Initialized"; return;
                case Started: w << "Started"; return;
                case Stopped: w << "Stopped"; return;
            }
            w << "ReplicatedStoreTransactionReplicatorThrottleState(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( ReplicatedStoreTransactionReplicatorThrottleState, FirstValidEnum, LastValidEnum )
    }
}

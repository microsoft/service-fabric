// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        namespace StoreTransactionReadIsolationLevel
        {
            enum Enum
            {
                Snapshot = 0,
                //ReadCommitted = 1,
                ReadRepeatable = 1,
            };
        }

        namespace LockingHints
        {
            enum Enum
            {
                None = 0,
                UpdateLock = 4,
            };
        }

    }
}

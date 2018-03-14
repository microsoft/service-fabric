// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    namespace AbortReason
    {
        enum Enum
        {
            Invalid = 0,

            UserAborted = 1,

            UserDisposed = 2,

            SystemAborted = 4,
        };
    }
}

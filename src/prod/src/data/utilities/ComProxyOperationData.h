// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // Helper method to convert Com based operation data to KTL based OperationData
        // used during secondary copy/replication pump code path
        //
        class ComProxyOperationData
        {
        public:

            // Copies the memory from the native buffers to a ktl based OperationData
            // TODO: We could avoid the copy in the future
            static NTSTATUS Create(
                __in ULONG count,
                __in FABRIC_OPERATION_DATA_BUFFER const * const buffers,
                __in KAllocator & allocator,
                __out OperationData::CSPtr & result);
        };
    }
}

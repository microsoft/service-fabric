// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        class OperationData;

        class CRC64
        {
        public:
            static ULONG64 ToCRC64(
                __in KBuffer const & buffer,
                __in ULONG32 offset,
                __in ULONG32 count);

            static ULONG64 ToCRC64(
                __in byte const value[],
                __in ULONG32 offset,
                __in ULONG32 count);

            static ULONG64 ToCRC64(
                __in OperationData const & operationData,
                __in ULONG32 offset,
                __in ULONG32 count);

            static::ULONG64 ToCRC64(
                __in KArray<KSharedPtr<const OperationData>> const & operationDataArray,
                __in ULONG32 offset,
                __in ULONG32 count);
        };
    }
}

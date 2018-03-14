// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        // Async ktl::Task result types for various Stream operations

        struct PhysicalLogStreamReadResults
        {
            KtlLogAsn ResultingAsn;
            ULONG ResultingMetadataSize;
            ULONGLONG Version;
            KIoBuffer::SPtr ResultingMetadata;
            KIoBuffer::SPtr ResultingIoPageAlignedBuffer;
        };

        struct PhysicalLogStreamQueryRecordResults
        {
            ULONGLONG Version;
            KtlLogStream::RecordDisposition Disposition;
            ULONG RecordSize;
            ULONGLONG DebugInfo1;
        };

        struct CurrentLogUsageInformation
        {
            ULONG PercentageLogUsage;
        };
    }
}

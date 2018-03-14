// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface ICompletedRecordsProcessor
        {
            K_SHARED_INTERFACE(ICompletedRecordsProcessor)
            K_WEAKREF_INTERFACE(ICompletedRecordsProcessor)

            public:
                virtual void ProcessedLogicalRecord(__in LogRecordLib::LogicalLogRecord & logicalRecord) noexcept = 0;

                virtual void ProcessedPhysicalRecord(__in LogRecordLib::PhysicalLogRecord & physicalRecord) noexcept = 0;
        };
    }
}

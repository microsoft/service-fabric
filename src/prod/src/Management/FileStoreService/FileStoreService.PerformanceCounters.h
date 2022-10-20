// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/PerformanceCounter.h"

namespace Management
{
    namespace FileStoreService
    {
        class FileStoreServiceCounters
        {
            DENY_COPY(FileStoreServiceCounters)

        public:

            BEGIN_COUNTER_SET_DEFINITION(
                L"673ECAF5-6799-4601-8207-3176311E96AD",
                L"File Store Service Component",
                L"Counters for File Store Service Component",
                Common::PerformanceCounterSetInstanceType::Multiple)

                COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"# of Create Upload Session Requests", L"Number of create upload session requests")
                COUNTER_DEFINITION(2, Common::PerformanceCounterType::RawData64, L"# of Commit Upload Session Requests", L"Number of commit upload session requests")
                COUNTER_DEFINITION(3, Common::PerformanceCounterType::RawData64, L"# of Delete Upload Session Requests", L"Number of delete upload session requests")
                COUNTER_DEFINITION(4, Common::PerformanceCounterType::RawData64, L"# of Successful Create Upload Session Requests", L"Number of successful create upload session requests")
                COUNTER_DEFINITION(5, Common::PerformanceCounterType::RawData64, L"# of Successful Commit Upload Session Requests", L"Number of successful commit upload session requests")
                COUNTER_DEFINITION(6, Common::PerformanceCounterType::RawData64, L"# of Successful Delete Upload Session Requests", L"Number of successful delete upload session requests")
                COUNTER_DEFINITION(7, Common::PerformanceCounterType::RawData64, L"Avg. Commit Upload Session Processing ms/KB/Operation", L"Average time for processing commit upload session requests")
                COUNTER_DEFINITION(8, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Delete Requests/sec", L"Delete requests per second")
                COUNTER_DEFINITION(9, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Internal List Requests/sec", L"Internal list requests per second")
                COUNTER_DEFINITION(10, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Pending commit operations/sec", L"Pending commit operations per second")
                COUNTER_DEFINITION(11, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Copy Action Requests/sec", L"Copy action requests per seconds")
                COUNTER_DEFINITION(12, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Upload Action Requests", L"Upload action requests")
                COUNTER_DEFINITION(13, Common::PerformanceCounterType::RawData64, L"# of non-transient failed Commit Upload Session requests", L"Number of non-transient failed commit upload session requests")
                COUNTER_DEFINITION(14, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Create Upload Session/sec", L"Successful create upload session requests per second")
                COUNTER_DEFINITION(15, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Commit Upload Session/sec", L"Successful commit upload session requests per second")
                COUNTER_DEFINITION(16, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Delete Upload Session/sec", L"Successful delete upload session requests per second")
                COUNTER_DEFINITION(17, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Upload Chunk Content request/sec", L"Successful upload chunk content per second")
                COUNTER_DEFINITION(18, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Create Upload Session request/sec", L"Create upload session requests per second")
                COUNTER_DEFINITION(19, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Commit Upload Session request/sec", L"Commit upload session requests per second")
                COUNTER_DEFINITION(20, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Delete Upload Session request/sec", L"Delete upload session requests per second")

            END_COUNTER_SET_DEFINITION()

            DECLARE_COUNTER_INSTANCE(NumberOfCreateUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfCommitUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfDeleteUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfSuccessfulCreateUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfSuccessfulCommitUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfSuccessfulDeleteUploadSessionRequests)
            DECLARE_AVERAGE_TIME_COUNTER_INSTANCE(AverageCommitUploadSessionProcessTime)
            DECLARE_COUNTER_INSTANCE(RateOfDeleteRequests)
            DECLARE_COUNTER_INSTANCE(RateOfInternalListRequests)
            DECLARE_COUNTER_INSTANCE(RateOfPendingCommitUploadOperations)
            DECLARE_COUNTER_INSTANCE(RateOfCopyActionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfUploadActionRequests)
            DECLARE_COUNTER_INSTANCE(NumberOfNonTransientFailedCommitUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulCreateUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulCommitUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulDeleteUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulUploadChunkContentRequests)
            DECLARE_COUNTER_INSTANCE(RateOfCreateUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfCommitUploadSessionRequests)
            DECLARE_COUNTER_INSTANCE(RateOfDeleteUploadSessionRequests)

            BEGIN_COUNTER_SET_INSTANCE(FileStoreServiceCounters)
                DEFINE_COUNTER_INSTANCE(NumberOfCreateUploadSessionRequests, 1)
                DEFINE_COUNTER_INSTANCE(NumberOfCommitUploadSessionRequests, 2)
                DEFINE_COUNTER_INSTANCE(NumberOfDeleteUploadSessionRequests, 3)
                DEFINE_COUNTER_INSTANCE(NumberOfSuccessfulCreateUploadSessionRequests, 4)
                DEFINE_COUNTER_INSTANCE(NumberOfSuccessfulCommitUploadSessionRequests, 5)
                DEFINE_COUNTER_INSTANCE(NumberOfSuccessfulDeleteUploadSessionRequests, 6)
                DEFINE_COUNTER_INSTANCE(AverageCommitUploadSessionProcessTime, 7)
                DEFINE_COUNTER_INSTANCE(RateOfDeleteRequests, 8)
                DEFINE_COUNTER_INSTANCE(RateOfInternalListRequests, 9)
                DEFINE_COUNTER_INSTANCE(RateOfPendingCommitUploadOperations, 10)
                DEFINE_COUNTER_INSTANCE(RateOfCopyActionRequests, 11)
                DEFINE_COUNTER_INSTANCE(RateOfUploadActionRequests, 12)
                DEFINE_COUNTER_INSTANCE(NumberOfNonTransientFailedCommitUploadSessionRequests, 13)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulCreateUploadSessionRequests, 14)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulCommitUploadSessionRequests, 15)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulDeleteUploadSessionRequests, 16)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulUploadChunkContentRequests, 17)
                DEFINE_COUNTER_INSTANCE(RateOfCreateUploadSessionRequests, 18)
                DEFINE_COUNTER_INSTANCE(RateOfCommitUploadSessionRequests, 19)
                DEFINE_COUNTER_INSTANCE(RateOfDeleteUploadSessionRequests, 20)
            END_COUNTER_SET_INSTANCE()

        public:
            void OnCreateUploadSessionRequest();
            void OnCommitUploadSessionRequest();
            void OnDeleteUploadSessionRequest();
            void OnSuccessfulDeleteUploadSessionRequest();
            void OnSuccessfulCreateUploadSessionRequest();
            void OnSuccessfulCommitUploadSessionRequest(int64 elapsedTicks);
            void OnSuccessfulUploadChunkContentRequest();
            void OnDeleteRequest();
            void OnInternalListRequest();
            void OnPendingCommitUploadSessionReply();
            void OnCopyActionRequest();
            void OnUploadActionRequest();
            void OnNonTransientFailedCommitUploadSessionRequest();
        };
    }
}

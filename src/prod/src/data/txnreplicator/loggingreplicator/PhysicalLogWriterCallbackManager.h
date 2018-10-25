// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // Used by the ILogRecordsDispatcher to dispatch records that get logged
        //
        class PhysicalLogWriterCallbackManager final
            : public Utilities::IDisposable
            , public KObject<PhysicalLogWriterCallbackManager>
            , public KShared<PhysicalLogWriterCallbackManager>
            , public KThreadPool::WorkItem
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(PhysicalLogWriterCallbackManager)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:
            static PhysicalLogWriterCallbackManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator & allocator);

            __declspec(property(get = get_FlushedPsn, put = set_FlushedPsn)) LONG64 FlushedPsn;
            LONG64 const get_FlushedPsn() const
            {
                return flushedPsn_;
            }

            void set_FlushedPsn(LONG64 value)
            {
                flushedPsn_ = value;
            }

            __declspec(property(get = get_FlushCallbackProcessor, put = set_FlushCallbackProcessor)) IFlushCallbackProcessor::SPtr FlushCallbackProcessor;
            IFlushCallbackProcessor::SPtr get_FlushCallbackProcessor()
            {
                IFlushCallbackProcessor::SPtr lock = flushCallbackProcessor_->TryGetTarget();
                return lock;
            }
            void set_FlushCallbackProcessor(IFlushCallbackProcessor & processor)
            {
                NTSTATUS status = processor.GetWeakIfRef(flushCallbackProcessor_);
                ASSERT_IFNOT(NT_SUCCESS(status), "Failed to get weak ref to flush callback processor in PhysicalLogWriterCallbackManager");
            }

            void Dispose() override;

            //
            // Invoked by the physical log writer to process the records that were flushed to disk
            //
            void ProcessFlushedRecords(__in LoggedRecords const & flushedRecords);

        private:

            PhysicalLogWriterCallbackManager(
                __in Utilities::PartitionedReplicaId const & traceId);

            void Execute();

            void TraceLoggingException(
                __in LogRecordLib::LogRecord const & record,
                __in NTSTATUS logError);

            bool isDisposed_;

            KSpinLock callbackLock_;
            bool isCallingback_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr loggedRecordsArray_;
            KSharedArray<LoggedRecords::CSPtr>::SPtr recordsToCallback_;
            KWeakRef<IFlushCallbackProcessor>::SPtr flushCallbackProcessor_;
            LONG64 flushedPsn_;
        };
    }
}

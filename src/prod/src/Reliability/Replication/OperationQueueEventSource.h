// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        #define DECLARE_OQ_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
        #define OQ_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::OperationQueue, \
                trace_id, \
                #trace_name, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        class OperationQueueEventSource
        {
        public:
            DECLARE_OQ_STRUCTURED_TRACE(Ctor, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG, ULONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Dtor, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Duplicate, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(DuplicateBatch, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(QueueFull, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(QueueMemoryFull, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Enqueue, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, int, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(EnqueueBatch, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Resize, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Complete, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Commit, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(ResetHead, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(UpdateCommitBack, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(UpdateCompleteBack, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(UpdateCompleteFront, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Skip, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Query, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(CancelOp, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Remove, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(CompleteLess, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(Gap, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, std::wstring);
            DECLARE_OQ_STRUCTURED_TRACE(ShrinkError, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, ULONGLONG, ULONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(DiscardPendingOps, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(OperationCommit, LONGLONG, int64);
            DECLARE_OQ_STRUCTURED_TRACE(OperationComplete, LONGLONG, int64);
            DECLARE_OQ_STRUCTURED_TRACE(OperationCleanup, LONGLONG, int64);
            DECLARE_OQ_STRUCTURED_TRACE(OperationAck, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(OperationAckCallback, Common::Guid, std::wstring, LONGLONG);
            DECLARE_OQ_STRUCTURED_TRACE(CompleteAckGap, Common::Guid, std::wstring, LONGLONG, LONGLONG, LONGLONG, LONGLONG, LONGLONG);
                        
            OperationQueueEventSource() :
                OQ_STRUCTURED_TRACE(Ctor, 4, Info, "Queue.ctor: {1} [{2}, {3}, {4}, {5}] maxSize={6}, maxMemorySize={7}, count={8}", "id", "description", "completedHead", "head", "committedHead", "tail", "maxSize", "maxMemorySize", "count"),
                OQ_STRUCTURED_TRACE(Dtor, 5, Info, "Queue.dtor: {1} [{2}, {3}, {4}, {5}] count={6}", "id", "description", "completedHead", "head", "committedHead", "tail", "count"),
                OQ_STRUCTURED_TRACE(Duplicate, 6, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Drop {6}: less than head or received out of order", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN"),
                OQ_STRUCTURED_TRACE(DuplicateBatch, 31, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Drop batch [{6}, {7}]: less than head or received out of order", "id", "description", "completedHead", "head", "committedHead", "tail", "firstLSN", "lastLSN"),
                OQ_STRUCTURED_TRACE(QueueFull, 7, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Drop {6}: queue full and expand failed (totalMemorySize={7})", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN", "totalMemorySize"),
                OQ_STRUCTURED_TRACE(QueueMemoryFull, 29, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Drop {6} (size={7}): memory limit is reached (totalMemorySize={8})", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN", "opSize", "totalMemorySize"),
                OQ_STRUCTURED_TRACE(Enqueue, 8, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Enqueue {6} (type {7}) totalMemorySize={8}", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN", "opType", "totalMemorySize"),
                OQ_STRUCTURED_TRACE(EnqueueBatch, 30, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Enqueue batch operation [{6}, {7}] totalMemorySize={8}", "id", "description", "completedHead", "head", "committedHead", "tail", "firstLSN", "lastLSN", "totalMemorySize"),
                OQ_STRUCTURED_TRACE(Resize, 9, Info, "Queue {1} [{2}, {3}, {4}, {5}]: Capacity changed from {6} to {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "oldCapacity", "newCapacity"),
                OQ_STRUCTURED_TRACE(Complete, 10, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Complete {6} to {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "oldLSN", "newLSN"),
                OQ_STRUCTURED_TRACE(Commit, 11, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Commit {6} to {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "oldLSN", "newLSN"),
                OQ_STRUCTURED_TRACE(ResetHead, 12, Info, "Queue {1} [{2}, {3}, {4}, {5}]: Reset {6} from {7} to {8}", "id", "description", "completedHead", "head", "committedHead", "tail", "index", "oldLSN", "newLSN"),
                OQ_STRUCTURED_TRACE(UpdateCommitBack, 32, Info, "Queue {1} [{2}, {3}, {4}, {5}]: {6} NOP as new LSN {7} is lesser than head", "id", "description", "completedHead", "head", "committedHead", "tail", "index", "newLSN"),
                OQ_STRUCTURED_TRACE(UpdateCompleteBack, 13, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: First available completed {6} greater or equal than {7}, NOP", "id", "description", "completedHead", "head", "committedHead", "tail", "oldLSN", "newLSN"),
                OQ_STRUCTURED_TRACE(UpdateCompleteFront, 14, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Completed up to {6}, can't clear up to {7}, NOP", "id", "description", "completedHead", "head", "committedHead", "tail", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(Skip, 15, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: No operations to {6} from {7} to {8}", "id", "description", "completedHead", "head", "committedHead", "tail", "function", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(Query, 16, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Get operations {6} to {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(CancelOp, 17, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: {6}: {7} is missing, stop", "id", "description", "completedHead", "head", "committedHead", "tail", "startLSN", "function", "LSN"),
                OQ_STRUCTURED_TRACE(Remove, 18, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Remove operations {6} to {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(CompleteLess, 19, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Not all items until {6} are committed, try to complete {7} to {8}", "id", "description", "completedHead", "head", "committedHead", "tail", "desiredLSN", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(Gap, 20, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: {6} is missing, stop {7}", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN", "function"),
                OQ_STRUCTURED_TRACE(ShrinkError, 21, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Shrink from capacity {6} is not possible: can't fit {7} items", "id", "description", "completedHead", "head", "committedHead", "tail", "capacity", "itemCount"),
                OQ_STRUCTURED_TRACE(DiscardPendingOps, 22, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: Discard pending operations {6} to {7} (gaps allowed)", "id", "description", "completedHead", "head", "committedHead", "tail", "startLSN", "endLSN"),
                OQ_STRUCTURED_TRACE(OperationCommit, 23, Noise, "{0} is committed after {1} ms", "LSN", "durationMs"),
                OQ_STRUCTURED_TRACE(OperationComplete, 24, Noise, "{0} is completed after {1} ms", "LSN", "durationMs"),
                OQ_STRUCTURED_TRACE(OperationCleanup, 25, Noise, "{0} is cleaned up after {1} ms", "LSN", "durationMs"),
                OQ_STRUCTURED_TRACE(OperationAck, 26, Noise, "{0} is ACKed", "LSN"),
                OQ_STRUCTURED_TRACE(OperationAckCallback, 27, Noise, "{1}: {2} is ACKed", "id", "description", "LSN"),
                OQ_STRUCTURED_TRACE(CompleteAckGap, 28, Noise, "Queue {1} [{2}, {3}, {4}, {5}]: {6} doesn't have ACK, stop completing operations", "id", "description", "completedHead", "head", "committedHead", "tail", "LSN")
            {
            }

            static Common::Global<OperationQueueEventSource> Events;

        }; // end OperationQueueEventSource

    } // end namespace ReplicationComponent
} // end namespace Reliability

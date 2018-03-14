// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class responsible for batch tracing Secondary receive/ack info
        class SecondaryReplicatorTraceHandler 
        {
            DENY_COPY(SecondaryReplicatorTraceHandler)

        public:
            SecondaryReplicatorTraceHandler(ULONG64 const & batchSize);

            // Return current size of pending trace messages vector
            __declspec(property(get = get_Size)) int Size;
            int get_Size() const { return (int)pendingTraceMessages_.size(); }

            // Return current index into pending trace messages vector
            __declspec(property(get = get_Index)) ULONG64 Index;
            ULONG64 get_Index() const { return pendingTraceMessageIndex_; }

            // Add element from batch operation to the array of messages
            // Note : This method is NOT thread safe and the caller must ensure GetTraceMessages is not executed simultaneously
            void AddBatchOperation(
                __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
                __in FABRIC_SEQUENCE_NUMBER const & highLSN);

            // Add single operation to array of messages. 
            // Used when appending Copy receive messages and internally by AddBatchOperation
            // Until configured size is reached, elements are appended to the vector
            // Afterwards, the oldest element in the vector is overwritten
            // Note : This method is NOT thread safe and the caller must ensure GetTraceMessages is not executed simultaneously
            void AddOperation(__in FABRIC_SEQUENCE_NUMBER const & lsn);

            // Return the std::vector<SecondaryReplicatorTraceInfo> that will be traced
            // Note : This method is NOT thread safe and the caller must ensure AddElementsFromBatch/AddOperation are not executed simultaneously
            std::vector<SecondaryReplicatorTraceInfo> GetTraceMessages();

            ErrorCode Test_GetLsnAtIndex(
                __in ULONG64 index,
                __out LONG64 & lowLsn,
                __out LONG64 & highLsn);

        private:

            void IncrementPendingTraceIndex();

            void AddOperationPrivate(
                __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
                __in FABRIC_SEQUENCE_NUMBER const & highLSN);

            ULONG64 const secondaryReplicatorBatchTracingArraySize_;
            ULONG64 pendingTraceMessageIndex_;
            std::vector<SecondaryReplicatorTraceInfo> pendingTraceMessages_;
        }; // end SecondaryReplicatorTraceHandler
    } // end namespace ReplicationComponent
} // end namespace Reliability

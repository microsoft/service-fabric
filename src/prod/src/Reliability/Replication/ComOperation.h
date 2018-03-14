// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Abstract class that implements IFabricOperation.
        // {4C10B881-070C-4F8F-8D6F-93E588FAD792}
        static const GUID CLSID_ComOperation = 
            { 0x4c10b881, 0x70c, 0x4f8f, { 0x8d, 0x6f, 0x93, 0xe5, 0x88, 0xfa, 0xd7, 0x92 } };

        class ComOperation : public IFabricOperation, public Common::ComUnknownBase
        {
            DENY_COPY(ComOperation)

        public:
                        
            virtual ~ComOperation();

            __declspec (property(get=get_MetadataProperty)) FABRIC_OPERATION_METADATA const & Metadata;
            FABRIC_OPERATION_METADATA const & get_MetadataProperty() const { return metadata_; }
            
            __declspec (property(get=get_Epoch)) FABRIC_EPOCH Epoch;
            FABRIC_EPOCH get_Epoch() const { return epoch_; }
            
            __declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER SequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return metadata_.SequenceNumber; }

            __declspec (property(get=get_DataSize)) ULONGLONG DataSize;
            ULONGLONG get_DataSize();

            __declspec (property(get=get_Type)) FABRIC_OPERATION_TYPE Type;
            FABRIC_OPERATION_TYPE get_Type() const { return metadata_.Type; }

            __declspec (property(get=get_EnqueueTime)) Common::StopwatchTime EnqueueTime;
            Common::StopwatchTime get_EnqueueTime() const { return enqueueTime_; }

            __declspec (property(get=get_NeedsAck)) bool NeedsAck;
            virtual bool get_NeedsAck() const { return false; }

            __declspec (property(get=get_LastOperationInBatch)) FABRIC_SEQUENCE_NUMBER LastOperationInBatch;
            virtual FABRIC_SEQUENCE_NUMBER get_LastOperationInBatch() { return lastOperationInBatch_; }

            __declspec (property(get=get_IsEndOfStreamOperation)) bool IsEndOfStreamOperation;
            bool get_IsEndOfStreamOperation() const { return metadata_.Type == FABRIC_OPERATION_TYPE_END_OF_STREAM; }

            virtual void SetIgnoreAckAndKeepParentAlive(Common::ComponentRoot const &) 
            { 
                Common::Assert::CodingError("SetIgnoreAckAndKeepParentAlive should be overridden in the derived classes that care about it"); 
            }

            // IFabricOperation methods
            virtual const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE get_Metadata(void);

            virtual HRESULT STDMETHODCALLTYPE GetData( 
                /* [out] */ ULONG *count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const **buffers) = 0;
        
            virtual HRESULT STDMETHODCALLTYPE Acknowledge() = 0;

            // Other methods used by Replication layer
            virtual bool IsEmpty() const = 0;

            // Mark the operation as committed, but not completed.
            // Depending on the queue type, commit can signify different things.
            Common::TimeSpan Commit();
        
            // Mark the operation as completed - the user doesn't need it anymore.
            // Based on the type of the container, the operation may still be kept 
            // and used for other purposes until Cleanup is called.
            Common::TimeSpan Complete();

            // The operation is not needed anymore, so it can be cleaned up.
            // Once cleaned up, the operation should be removed from the queue.
            Common::TimeSpan Cleanup();
            
            // Refreshes the enqueue time to the current time
            void RefreshEnqueueTime()
            {
                enqueueTime_ = Common::Stopwatch::Now();
            }

            virtual void OnAckCallbackStartedRunning()
            {
                Common::Assert::CodingError("OnAckCallbackStartedRunning should be overridden in the derived classes that care about it"); 
            }

        protected:
            ComOperation(
                FABRIC_OPERATION_METADATA const & metadata,
                FABRIC_EPOCH const & epoch,
                FABRIC_SEQUENCE_NUMBER lastOperationInBatch);

        private:
            // Calculate the data size in the operation
            ULONGLONG GetDataSize();

            ULONGLONG dataSize_;
            FABRIC_OPERATION_METADATA metadata_;
            FABRIC_EPOCH epoch_;
            Common::StopwatchTime enqueueTime_;
            Common::StopwatchTime commitTime_;
            Common::StopwatchTime completeTime_;
            Common::StopwatchTime cleanupTime_;

            FABRIC_SEQUENCE_NUMBER lastOperationInBatch_;

        }; // end ComOperation

    } // end namespace ReplicationComponent
} // end namespace Reliability

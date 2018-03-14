// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Implementation of IFabricOperation created with the bytes of
        // the message body.
        class ComFromBytesOperation : public ComOperation
        {
            DENY_COPY(ComFromBytesOperation)

            COM_INTERFACE_LIST2(
                ComFromBytesOperation,
                IID_IFabricOperation,
                IFabricOperation,
                CLSID_ComOperation,
                ComOperation)

        public:
            static ComOperationCPtr CreateEndOfStreamOperationAndKeepRootAlive(OperationAckCallback const & ackCallback, Common::ComponentRoot const & root)
            {
                FABRIC_OPERATION_METADATA metaData;
                metaData.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
                metaData.Type = FABRIC_OPERATION_TYPE_END_OF_STREAM;
                metaData.SequenceNumber = Constants::InvalidLSN;
                metaData.Reserved = nullptr;

                ComOperationCPtr eosOperation = Common::make_com<ComFromBytesOperation, ComOperation>(
                    metaData,
                    ackCallback,
                    Constants::InvalidLSN,
                    root);
                
                return eosOperation;
            }

            virtual ~ComFromBytesOperation();

            virtual bool get_NeedsAck() const;

            virtual void SetIgnoreAckAndKeepParentAlive(Common::ComponentRoot const & root);

            virtual HRESULT STDMETHODCALLTYPE GetData( 
                /* [out] */ ULONG * count,
                /* [retval][out] */ FABRIC_OPERATION_DATA_BUFFER const ** buffers);

            virtual HRESULT STDMETHODCALLTYPE Acknowledge();

            virtual bool IsEmpty() const;

            virtual void OnAckCallbackStartedRunning() override;

        private:
            ComFromBytesOperation(
                std::vector<Common::const_buffer> const & buffers,
                std::vector<ULONG> const & segmentSizes,
                FABRIC_OPERATION_METADATA const & metadata,
                OperationAckCallback const & ackCallback,
                FABRIC_SEQUENCE_NUMBER lastOperationInBatch);

            ComFromBytesOperation(
                std::vector<Common::const_buffer> const & buffers,
                std::vector<ULONG> const & segmentSizes,
                FABRIC_OPERATION_METADATA const & metadata,
                FABRIC_EPOCH const & epoch,
                OperationAckCallback const & ackCallback,
                FABRIC_SEQUENCE_NUMBER lastOperationInBatch);

            ComFromBytesOperation(
                std::vector<BYTE> && data,
                std::vector<ULONG> const & segmentSizes,
                FABRIC_OPERATION_METADATA const & metadata,
                FABRIC_EPOCH const & epoch,
                OperationAckCallback const & ackCallback,
                FABRIC_SEQUENCE_NUMBER lastOperationInBatch);

            ComFromBytesOperation(
                FABRIC_OPERATION_METADATA const & metadata,
                OperationAckCallback const & ackCallback,
                FABRIC_SEQUENCE_NUMBER lastOperationInBatch,
                Common::ComponentRoot const & root);


            void InitBuffer(std::vector<Common::const_buffer> const & buffers);

            void InitData();

            template <class ComImplementation, class T0, class T1>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation, class ComInterface, class T0, class T1>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1);

            template <class ComImplementation, class T0, class T1, class T2>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1, T2 && a2);

            template <class ComImplementation, class ComInterface, class T0, class T1, class T2>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2);

            template <class ComImplementation, class T0, class T1, class T2, class T3>
            friend Common::ComPointer<ComImplementation> Common::make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3);

            template <class ComImplementation, class ComInterface, class T0, class T1, class T2, class T3>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3);

            template <class ComImplementation, class ComInterface, class T0, class T1, class T2, class T3, class T4>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4);

            template <class TComImplementation, class ComInterface, class T0, class T1, class T2, class T3, class T4, class T5>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5);

            template <class TComImplementation, class ComInterface, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
            friend Common::ComPointer<ComInterface> Common::make_com(T0 && a0, T1 && a1, T2 && a2, T3 && a3, T4 && a4, T5 && a5, T6 && a6);

            std::vector<BYTE> data_;
            std::vector<ULONG> segmentSizes_;
            std::vector<FABRIC_OPERATION_DATA_BUFFER> segments_;

            // The callback should be called as long as the replicator 
            // that created the operation exists.
            // When closing the replicator, we must make sure all operations 
            // are either ACKed, or discarded
            OperationAckCallback ackCallback_;

            enum CallbackState
            {
                NotCalled = 0,
                ReadyToRun = 1,
                Running = 2,
                Completed = 3,
                Cancelled = 4,
            };

            CallbackState callbackState_;

            Common::ComponentRootSPtr rootSPtr_;
            MUTABLE_RWLOCK(REComFromBytesOperation, lock_);

        }; // end ComFromBytesOperation
    } // end namespace ReplicationComponent
} // end namespace Reliability

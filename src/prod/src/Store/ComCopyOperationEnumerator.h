// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ComCopyOperationEnumerator
        : public IFabricOperationDataStream 
        , public Common::ComUnknownBase
        , public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        COM_INTERFACE_LIST1(
            ComCopyOperationEnumerator,
            IID_IFabricOperationDataStream,
            IFabricOperationDataStream)

    public:
        static Common::ErrorCode CreateForRemoteCopy(
            __in ReplicatedStore & replicatedStore,
            std::vector<ProgressVectorEntry> && progressVector,
            ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
            Common::ComPointer<IFabricOperationDataStream> const & copyContext,
            Common::ComponentRoot const & root,
            __out Common::ComPointer<ComCopyOperationEnumerator> &);

        static Common::ErrorCode CreateForLocalCopy(
            __in ReplicatedStore & replicatedStore,
            ILocalStoreUPtr const & localStore,
            FABRIC_SEQUENCE_NUMBER copyStartSequenceNumber,
            Common::ComponentRoot const & root,
            __out Common::ComPointer<ComCopyOperationEnumerator> &);

        ~ComCopyOperationEnumerator();

        __declspec(property(get=get_IsHoldingLowWatermarkReadLock, put=set_IsHoldingLowWatermarkReadLock)) bool IsHoldingLowWatermarkReadLock;
        bool get_IsHoldingLowWatermarkReadLock() const { return isHoldingLowWatermarkReadLock_; }
        void set_IsHoldingLowWatermarkReadLock(bool isHoldingLowWatermarkReadLock) { isHoldingLowWatermarkReadLock_ = isHoldingLowWatermarkReadLock; }

        __declspec(property(get=get_LocalStore)) ILocalStoreUPtr const & LocalStore;
        ILocalStoreUPtr const & get_LocalStore() const { return localStoreUPtr_; }

        // *** IFabricOperationDataStream
        //
        HRESULT STDMETHODCALLTYPE BeginGetNext(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
        HRESULT STDMETHODCALLTYPE EndGetNext(__in IFabricAsyncOperationContext *, __out IFabricOperationData **);

        // *** Internal usage
        //
        Common::ErrorCode GetNextCopyOperationForLocalApply(__out CopyOperation &);
            
    private:
        class ComGetCopyContextAsyncOperation;
        class ComCopyOperationContext;

        ComCopyOperationEnumerator(
            __in ReplicatedStore & replicatedStore,
            std::vector<ProgressVectorEntry> && progressVector,
            ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
            Common::ComPointer<IFabricOperationDataStream> const & copyContext,
            Common::ComponentRoot const & root);

        ComCopyOperationEnumerator(
            __in ReplicatedStore & replicatedStore,
            ILocalStoreUPtr const & localStore,
            IStoreBase::TransactionSPtr &&,
            FABRIC_SEQUENCE_NUMBER copyStartSequenceNumber,
            std::vector<ProgressVectorEntry> && progressVector,
            ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
            Common::ComponentRoot const & root);

        void AcquireLowWatermarkReaderCount();
        void ReleaseLowWatermarkReaderCount();

        ReplicatedStore & replicatedStore_;
        ILocalStoreUPtr const & localStoreUPtr_;
        IStoreBase::TransactionSPtr txSPtr_;
        std::vector<ProgressVectorEntry> progressVector_;
        ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN_;
        Common::ComPointer<IFabricOperationDataStream> copyContextCPtr_;
        Common::ComponentRootSPtr root_; 

        int currentCopyProgressVectorIndex_;
        ::FABRIC_SEQUENCE_NUMBER lastEnumeratedOperationLSN_;
        bool epochHistorySent_;
        bool lowWatermarkSent_;
        bool isFirstGetNext_;
        bool isHoldingLowWatermarkReadLock_;

        bool isFileStreamFullCopy_;
        bool isLogicalFullCopy_;
        FileStreamFullCopyContextSPtr fileStreamFullCopyContextSPtr_;
    };
}

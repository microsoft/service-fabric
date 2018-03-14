// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("ComCopyOperationEnumerator");

    class ComCopyOperationEnumerator::ComGetCopyContextAsyncOperation : public ComProxyAsyncOperation
    {
        DENY_COPY(ComGetCopyContextAsyncOperation);

    public:
        ComGetCopyContextAsyncOperation(
            ComPointer<IFabricOperationDataStream> const & copyContext,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & root);

        static ErrorCode End(AsyncOperationSPtr const &, __out ComPointer<IFabricOperationData> &);

    protected:
        HRESULT BeginComAsyncOperation(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
        HRESULT EndComAsyncOperation(__in IFabricAsyncOperationContext *);

    private:
        ComPointer<IFabricOperationDataStream> copyContextCPtr_;
        ComPointer<IFabricOperationData> operationData_;
    };

    class ComCopyOperationEnumerator::ComCopyOperationContext : public ComAsyncOperationContext
    {
        DENY_COPY(ComCopyOperationContext);

    public:
        ComCopyOperationContext(__in ComCopyOperationEnumerator & owner)
            : copyOperation_()
            , waitForLsnRetryCount_(0)
            , stopwatch_()
        {
            owner_.SetAndAddRef(&owner);
            WriteNoise(
                TraceComponent,
                "{0}: {1} ComCopyOperationContext::ctor",
                owner_->TraceId,
                static_cast<void*>(this));

            owner_->replicatedStore_.PerfCounters.RateOfCopy.Increment();

            stopwatch_.Start();
        }

        ~ComCopyOperationContext()
        {
            WriteNoise(
                TraceComponent,
                "{0}: {1} ComCopyOperationContext::~dtor",
                owner_->TraceId,
                static_cast<void*>(this));
        }

        HRESULT STDMETHODCALLTYPE End(__out ComPointer<ComReplicatedOperationData> & operation);

        // Internal usage
        //
        ErrorCode GetNextCopyOperationForLocalApply(__out CopyOperation &);

    protected:
        void OnStart(AsyncOperationSPtr const &);

    private:

        bool IsPhysicalFullCopyEnabled();

        ErrorCode InitializeCopyOperation(
            CopyContextData const & copyContext,
            __out bool & doFullCopy,
            __out ::FABRIC_SEQUENCE_NUMBER & copyStartSequenceNumber);

        void OnGetCopyContextComplete(AsyncOperationSPtr const &);

        void ScheduleGetFileStreamFullCopyContext(AsyncOperationSPtr const &);
        void GetFileStreamFullCopyContext(AsyncOperationSPtr const &);
        void OnGetFileStreamFullCopyContextComplete(AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
        ErrorCode ReadNextFileStreamChunk(bool isFirstChunk);

        ErrorCode EnumerateCopyOperations(
            ILocalStore::TransactionSPtr const & txSPtr,
            ::FABRIC_SEQUENCE_NUMBER enumerationStartOperationLSN,
            CopyType::Enum);

        ErrorCode CreateComReplicatedOperationData(__out ComPointer<ComReplicatedOperationData> & result);

        void ScheduleWaitForUpToOperationLSN(AsyncOperationSPtr const & proxySPtr);

        int GetCopyEpochIndex(::FABRIC_SEQUENCE_NUMBER lastEnumeratedOperationLSN);

        ErrorCode GetLowWatermark(
            ILocalStore::TransactionSPtr const&,
            __out TombstoneLowWatermarkData &);

        bool TryGetCurrentEpochUpdate(
            ::FABRIC_SEQUENCE_NUMBER lastEnumeratedOperationLSN,
            __out ReplicationOperation &,
            __out ErrorCode &);

        ErrorCode GetCurrentEpochUpdate(
            size_t progressVectorIndex,
            __out ReplicationOperation &);

        bool TryGetEpochHistoryUpdate(
            ILocalStore::TransactionSPtr const&,
            __out ReplicationOperation &,
            __out ErrorCode &);

        bool TryGetLowWatermarkUpdate(
            ILocalStore::TransactionSPtr const&,
            __out ReplicationOperation &,
            __out ErrorCode &);

        ComPointer<ComCopyOperationEnumerator> owner_;
        CopyOperation copyOperation_;
        int waitForLsnRetryCount_;
        Stopwatch stopwatch_;
    };

    // **************************
    // ComCopyOperationEnumerator
    // **************************

    ComCopyOperationEnumerator::ComCopyOperationEnumerator(
        __in ReplicatedStore & replicatedStore,
        std::vector<ProgressVectorEntry> && progressVector,
        ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
        Common::ComPointer<IFabricOperationDataStream> const & copyContext,
        Common::ComponentRoot const & root)
        : ReplicaActivityTraceComponent(replicatedStore.PartitionedReplicaId, Common::ActivityId())
        , replicatedStore_(replicatedStore)
        , localStoreUPtr_(replicatedStore.LocalStore)
        , txSPtr_()
        , progressVector_(move(progressVector))
        , uptoOperationLSN_(uptoOperationLSN)
        , copyContextCPtr_(copyContext)
        , root_(root.CreateComponentRoot())
        , currentCopyProgressVectorIndex_(-1)
        , lastEnumeratedOperationLSN_(0)
        , epochHistorySent_(false)
        , lowWatermarkSent_(false)
        , isFirstGetNext_(true)
        , isHoldingLowWatermarkReadLock_(false)
        , isFileStreamFullCopy_(false)
        , isLogicalFullCopy_(false)
        , fileStreamFullCopyContextSPtr_()
    {
        WriteNoise(
            TraceComponent,
            "{0} ctor: this={1} uptoOperationLSN={2}",
            this->TraceId,
            static_cast<void*>(this),
            uptoOperationLSN);

        ASSERT_IFNOT(progressVector_.size() >= 1, "{0} progress vector must contain at least current epoch", this->TraceId);
        ASSERT_IFNOT(copyContextCPtr_, "{0} Service should be using a non-NULL copy context", this->TraceId);
    }

    ComCopyOperationEnumerator::ComCopyOperationEnumerator(
        __in ReplicatedStore & replicatedStore,
        ILocalStoreUPtr const & localStore,
        IStoreBase::TransactionSPtr && txSPtr,
        ::FABRIC_SEQUENCE_NUMBER copyStartSequenceNumber,
        std::vector<ProgressVectorEntry> && progressVector,
        ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
        Common::ComponentRoot const & root)
        : ReplicaActivityTraceComponent(replicatedStore.PartitionedReplicaId, Common::ActivityId())
        , replicatedStore_(replicatedStore)
        , localStoreUPtr_(localStore)
        , txSPtr_(move(txSPtr))
        , progressVector_(move(progressVector))
        , uptoOperationLSN_(uptoOperationLSN)
        , copyContextCPtr_()
        , root_(root.CreateComponentRoot())
        , currentCopyProgressVectorIndex_(-1)
        , lastEnumeratedOperationLSN_(copyStartSequenceNumber)
        , epochHistorySent_(false)
        , lowWatermarkSent_(false)
        , isFirstGetNext_(true)
        , isHoldingLowWatermarkReadLock_(false)
        , isFileStreamFullCopy_(false)
        , isLogicalFullCopy_(false)
        , fileStreamFullCopyContextSPtr_()
    {
        WriteNoise(
            TraceComponent,
            "{0} ctor: this={1} uptoOperationLSN={2}",
            this->TraceId,
            static_cast<void*>(this),
            uptoOperationLSN);
    }

    ComCopyOperationEnumerator::~ComCopyOperationEnumerator()
    {
        WriteNoise(
            TraceComponent,
            "{0} ~dtor: this={1}",
            this->TraceId,
            static_cast<void*>(this));

        this->ReleaseLowWatermarkReaderCount();
    }

    void ComCopyOperationEnumerator::AcquireLowWatermarkReaderCount()
    {
        replicatedStore_.IncrementReaderCounterLocked();

        this->IsHoldingLowWatermarkReadLock = true;
    }

    void ComCopyOperationEnumerator::ReleaseLowWatermarkReaderCount()
    {
        if (this->IsHoldingLowWatermarkReadLock)
        {
            this->IsHoldingLowWatermarkReadLock = false;

            replicatedStore_.DecrementReaderCounterLocked();

            WriteInfo(
                TraceComponent,
                "{0}: released low watermark reader count",
                this->TraceId);
        }
    }

    ErrorCode ComCopyOperationEnumerator::CreateForRemoteCopy(
        __in ReplicatedStore & replicatedStore,
        std::vector<ProgressVectorEntry> && progressVector,
        ::FABRIC_SEQUENCE_NUMBER uptoOperationLSN,
        Common::ComPointer<IFabricOperationDataStream> const & copyContext,
        Common::ComponentRoot const & root,
        __out ComPointer<ComCopyOperationEnumerator> & resultCPtr)
    {
        resultCPtr.SetNoAddRef(new ComCopyOperationEnumerator(
            replicatedStore,
            move(progressVector),
            uptoOperationLSN,
            copyContext,
            root));

        return ErrorCodeValue::Success;
    }

    ErrorCode ComCopyOperationEnumerator::CreateForLocalCopy(
        __in ReplicatedStore & replicatedStore,
        ILocalStoreUPtr const & localStore,
        FABRIC_SEQUENCE_NUMBER copyStartSequenceNumber,
        Common::ComponentRoot const & root,
        __out ComPointer<ComCopyOperationEnumerator> & resultCPtr)
    {
        IStoreBase::TransactionSPtr txSPtr;
        auto error = replicatedStore.CreateLocalStoreTransaction(localStore, txSPtr);

        if (!error.IsSuccess()) { return error; }

        FABRIC_SEQUENCE_NUMBER operationLSN;
        error = localStore->GetLastChangeOperationLSN(
            txSPtr,
            operationLSN);

        if (!error.IsSuccess()) { return error; }

        vector<ProgressVectorEntry> progressVector;
        error = replicatedStore.GetProgressVector(
            localStore,
            make_unique<Store::DataSerializer>(replicatedStore.PartitionedReplicaId, *localStore),
            operationLSN,
            progressVector);

        if (!error.IsSuccess()) { return error; }

        resultCPtr.SetNoAddRef(new ComCopyOperationEnumerator(
            replicatedStore,
            localStore,
            move(txSPtr),
            copyStartSequenceNumber,
            move(progressVector),
            operationLSN,
            root));

        return error;
    }

    HRESULT STDMETHODCALLTYPE ComCopyOperationEnumerator::BeginGetNext(
        __in IFabricAsyncOperationCallback * callback,
        __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComAsyncOperationContextCPtr copyOperationContext = make_com<ComCopyOperationContext,ComAsyncOperationContext>(*this);

        HRESULT hr = copyOperationContext->Initialize(root_->CreateComponentRoot(), callback);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        hr = copyOperationContext->Start(copyOperationContext);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        hr = copyOperationContext->QueryInterface(
            IID_IFabricAsyncOperationContext,
            reinterpret_cast<void**>(operationContext));
        return ComUtility::OnPublicApiReturn(hr);
    }

    HRESULT STDMETHODCALLTYPE ComCopyOperationEnumerator::EndGetNext(
        __in IFabricAsyncOperationContext * operationContext,
        __out IFabricOperationData ** result)
    {
        if (operationContext == NULL || result == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        ComPointer<ComReplicatedOperationData> replicatedOperationData;
        ComPointer<ComCopyOperationContext> typedContext(operationContext, IID_IFabricAsyncOperationContext);

        HRESULT hr = typedContext->End(replicatedOperationData);
        if (!SUCCEEDED(hr)) { return ComUtility::OnPublicApiReturn(hr); }

        if (replicatedOperationData->IsEmpty)
        {
            *result = NULL;
            return ComUtility::OnPublicApiReturn(S_OK);
        }
        else
        {
            hr = replicatedOperationData->QueryInterface(IID_IFabricOperationData, reinterpret_cast<void**>(result));
            return ComUtility::OnPublicApiReturn(hr);
        }
    }

    ErrorCode ComCopyOperationEnumerator::GetNextCopyOperationForLocalApply(__out CopyOperation & result)
    {
        auto copyOperationContext = make_com<ComCopyOperationContext>(*this);

        return copyOperationContext->GetNextCopyOperationForLocalApply(result);
    }

    // *******************************
    // ComGetCopyContextAsyncOperation
    // *******************************

    ComCopyOperationEnumerator::ComGetCopyContextAsyncOperation::ComGetCopyContextAsyncOperation(
        ComPointer<IFabricOperationDataStream> const & copyContext,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ComProxyAsyncOperation(callback, root)
        , copyContextCPtr_(copyContext)
        , operationData_()
    {
    }

    ErrorCode ComCopyOperationEnumerator::ComGetCopyContextAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out ComPointer<IFabricOperationData> & result)
    {
        auto casted = AsyncOperation::End<ComGetCopyContextAsyncOperation>(asyncOperation);

        if (casted->Error.IsSuccess())
        {
            result.Swap(casted->operationData_);
        }

        return casted->Error;
    }

    HRESULT ComCopyOperationEnumerator::ComGetCopyContextAsyncOperation::BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback,
        __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        return ComUtility::OnPublicApiReturn(copyContextCPtr_->BeginGetNext(callback, operationContext));
    }

    HRESULT ComCopyOperationEnumerator::ComGetCopyContextAsyncOperation::EndComAsyncOperation(
        __in IFabricAsyncOperationContext * operationContext)
    {
        if (operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        return ComUtility::OnPublicApiReturn(copyContextCPtr_->EndGetNext(operationContext, operationData_.InitializationAddress()));
    }

    // ***********************
    // ComCopyOperationContext
    // ***********************

    void ComCopyOperationEnumerator::ComCopyOperationContext::OnStart(AsyncOperationSPtr const & proxySPtr)
    {
        // The first call to GetNext() will first retrieve the copy context to determine whether
        // or not a full copy is needed. Subsequent calls to GetNext() will simply continue
        // reading data items to continue the copy.
        //
        if (owner_->isFirstGetNext_)
        {
            owner_->isFirstGetNext_ = false;

            ScheduleWaitForUpToOperationLSN(proxySPtr);
        }
        else if (owner_->isFileStreamFullCopy_)
        {
            auto error = ReadNextFileStreamChunk(false); // isFirstChunk

            this->TryComplete(proxySPtr, error);
        }
        else
        {
            auto error = EnumerateCopyOperations(
                owner_->txSPtr_,
                owner_->lastEnumeratedOperationLSN_,
                CopyType::PagedCopy);

            this->TryComplete(proxySPtr, error);
        }
    }

    void ComCopyOperationEnumerator::ComCopyOperationContext::ScheduleWaitForUpToOperationLSN(
        AsyncOperationSPtr const & proxySPtr)
    {
        ::FABRIC_SEQUENCE_NUMBER lastOperationLSN = -1;
        auto error = owner_->replicatedStore_.GetLastCommittedSequenceNumber(lastOperationLSN);

        WriteNoise(
            TraceComponent,
            "{0}: checked last sequence number for copy state: requested = {1} committed = {2} error = {3}",
            owner_->TraceId,
            owner_->uptoOperationLSN_,
            lastOperationLSN,
            error);

        if (error.IsSuccess())
        {
            if (lastOperationLSN >= owner_->uptoOperationLSN_)
            {
                AsyncOperation::CreateAndStart<ComGetCopyContextAsyncOperation>(
                    owner_->copyContextCPtr_,
                    [this](AsyncOperationSPtr const & operation) { this->OnGetCopyContextComplete(operation); },
                    proxySPtr);
            }
            else if (++waitForLsnRetryCount_ < StoreConfig::GetConfig().MaxWaitForCopyLsnRetry)
            {
                // This can happen in the small window after a replicated operation is quorum-acked
                // but before the primary has persisted that operation to disk. Since the operation got
                // quorum-acked, it must eventually get persisted to disk. Schedule a retry to check for
                // the requested uptoOperationLSN value.
                //
                auto root = owner_->root_->CreateAsyncOperationRoot();
                Threadpool::Post(
                    [this, proxySPtr, root]() { this->ScheduleWaitForUpToOperationLSN(proxySPtr); },
                    TimeSpan::FromMilliseconds(StoreConfig::GetConfig().WaitForCopyLsnRetryDelayInMillis));
            }
            else
            {
                TRACE_ERROR_AND_ASSERT(
                    TraceComponent,
                    "{0}: exhausted retries while waiting for copy LSN: retries={1} request={2} committed={3}",
                    owner_->TraceId,
                    waitForLsnRetryCount_,
                    owner_->uptoOperationLSN_,
                    lastOperationLSN);
            }
        }
        else
        {
            this->TryComplete(proxySPtr, error.ToHResult());
        }
    }

    void ComCopyOperationEnumerator::ComCopyOperationContext::OnGetCopyContextComplete(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto const & thisSPtr = asyncOperation->Parent;

        ComPointer<IFabricOperationData> copyContextData;

        auto error = ComGetCopyContextAsyncOperation::End(asyncOperation, copyContextData);

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: read copy context failed: error = {1}",
                owner_->TraceId,
                error);

            this->TryComplete(thisSPtr, error);

            return;
        }

        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;

        if (copyContextData.GetRawPointer() != NULL)
        {
            HRESULT hr = copyContextData->GetData(&bufferCount, &replicaBuffers);

            if (!SUCCEEDED(hr))
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: read copy context data failed: hr = {1}",
                    owner_->TraceId,
                    hr);

                this->TryComplete(thisSPtr, hr);

                return;
            }
        }

        CopyContextData copyContext;

        if (bufferCount > 0)
        {
            vector<Serialization::FabricIOBuffer> buffers;
            ULONG totalSize = 0;
            for (ULONG i = 0; i < bufferCount; i++)
            {
                totalSize += replicaBuffers[i].BufferSize;
                buffers.push_back(Serialization::FabricIOBuffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
            }

            error = FabricSerializer::Deserialize(copyContext, totalSize, buffers);

            if (!error.IsSuccess())
            {
                this->TryComplete(thisSPtr, error);

                return;
            }
        }

        // Block tombstone truncation while there are active copy operations
        // to keep secondary tombstones more in sync with the primary and
        // ensure that full/partial copy calculations remain valid between
        // now and actually starting the copy.
        //
        owner_->AcquireLowWatermarkReaderCount();

        bool doFullCopy = true;
        ::FABRIC_SEQUENCE_NUMBER copyStartSequenceNumber = 0;

        error = this->InitializeCopyOperation(copyContext, doFullCopy, copyStartSequenceNumber);

        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);

            return;
        }

        owner_->copyContextCPtr_.SetNoAddRef(NULL);

        if (error.IsSuccess())
        {
            if (doFullCopy)
            {
                // Tombstone low watermark does not matter once we've decided to
                // perform a full copy. Release the reader count as soon as possible 
                // to avoid blocking tombstone truncation any longer than necessary.
                //
                owner_->ReleaseLowWatermarkReaderCount();

                copyStartSequenceNumber = 0;
                owner_->replicatedStore_.CopyStatistics.OnFullCopy();
            }
            else
            {
                owner_->replicatedStore_.CopyStatistics.OnPartialCopy();
            }

            WriteInfo(
                TraceComponent,
                "{0}: starting {1} copy ({2}): target=[{3}.{4:X}.{5}] start from {6} partial/full copy count = {7}/{8}",
                owner_->TraceId,
                doFullCopy ? "full" : "partial",
                copyContext.Id,
                copyContext.Epoch.DataLossNumber,
                copyContext.Epoch.ConfigurationNumber,
                copyContext.LastOperationLSN,
                copyStartSequenceNumber,
                owner_->replicatedStore_.CopyStatistics.GetFullCopyCount(),
                owner_->replicatedStore_.CopyStatistics.GetPartialCopyCount());

            if (doFullCopy &&
                owner_->uptoOperationLSN_ > 0 && // Optimization: avoid streaming empty database files
                copyContext.IsFileStreamFullCopySupported &&
                this->IsPhysicalFullCopyEnabled())
            {
                owner_->isFileStreamFullCopy_ = true;

                // Release long-running transaction (not needed for file stream full copy)
                //
                owner_->txSPtr_.reset();

                GetFileStreamFullCopyContext(thisSPtr);

                // FileStreamFullCopyContext constructed asynchronously
                //
                return;
            }
            else
            {
                owner_->isFileStreamFullCopy_ = false;
                owner_->isLogicalFullCopy_ = doFullCopy;

                error = EnumerateCopyOperations(
                    owner_->txSPtr_,
                    copyStartSequenceNumber,
                    doFullCopy ? CopyType::FirstFullCopy : CopyType::FirstPartialCopy);
            }
        }

        if (!error.IsSuccess())
        {
            owner_->isFirstGetNext_ = false;
        }

        this->TryComplete(asyncOperation->Parent, error.ToHResult());
    }

    bool ComCopyOperationEnumerator::ComCopyOperationContext::IsPhysicalFullCopyEnabled()
    {
        switch (owner_->replicatedStore_.Settings.FullCopyMode)
        {
        case FullCopyMode::Physical:
        case FullCopyMode::Rebuild:
            return true;

        case FullCopyMode::Logical:
            return false;

        default:
            return StoreConfig::GetConfig().EnableFileStreamFullCopy;
        }
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::InitializeCopyOperation(
        CopyContextData const & copyContext,
        __out bool & doFullCopy,
        __out ::FABRIC_SEQUENCE_NUMBER & copyStartSequenceNumber)
    {
        auto error = owner_->replicatedStore_.CreateLocalStoreTransaction(owner_->txSPtr_);

        if (!error.IsSuccess()) { return error; }

        // Default to full copy
        //
        doFullCopy = true;
        copyStartSequenceNumber = 0;

        if (!copyContext.IsEpochValid)
        {
            WriteInfo(
                TraceComponent,
                "{0}: invalid secondary epoch ({1}): [{2}.{3:X}.{4}]",
                owner_->TraceId,
                copyContext.Id,
                copyContext.Epoch.DataLossNumber,
                copyContext.Epoch.ConfigurationNumber,
                copyContext.LastOperationLSN);

            return ErrorCodeValue::Success;
        }

        if (copyContext.LastOperationLSN <= 0)
        {
            WriteInfo(
                TraceComponent,
                "{0}: empty secondary ({1}): [{2}.{3:X}.{4}]",
                owner_->TraceId,
                copyContext.Id,
                copyContext.Epoch.DataLossNumber,
                copyContext.Epoch.ConfigurationNumber,
                copyContext.LastOperationLSN);

            return ErrorCodeValue::Success;
        }

        // **************************************************************************
        // Search backwards through progress vector to determine whether or not a
        // partial copy is possible. The secondary must prepare for a full copy if one
        // of the following is true:
        //
        //   1) The secondary has made false progress
        //   2) The secondary has fallen too far behind
        //   3) The secondary is in an unrecognized epoch
        //   4) The secondary did not send any context data
        //
        // **************************************************************************

        ASSERT_IF(owner_->progressVector_.empty(), "{0} empty primary progress vector", owner_->TraceId);

        auto const & primaryCurrentEpoch = owner_->progressVector_.back().Epoch;
        auto const & secondaryCurrentEpoch = copyContext.Epoch;
        ASSERT_IF(
            primaryCurrentEpoch.DataLossNumber < secondaryCurrentEpoch.DataLossNumber
            || (primaryCurrentEpoch.DataLossNumber == secondaryCurrentEpoch.DataLossNumber && primaryCurrentEpoch.ConfigurationNumber < secondaryCurrentEpoch.ConfigurationNumber),
            "{0} primary epoch less than secondary epoch: ({1}.{2:X} < {3}.{4:X})", 
            owner_->TraceId,
            primaryCurrentEpoch.DataLossNumber,
            primaryCurrentEpoch.ConfigurationNumber,
            secondaryCurrentEpoch.DataLossNumber,
            secondaryCurrentEpoch.ConfigurationNumber);

        wstring pvTrace;
        StringWriter pvTraceString(pvTrace);

        for (auto ix = 0; ix < owner_->progressVector_.size(); ++ix)
        {
            ProgressVectorEntry const & entry = owner_->progressVector_[ix];
            pvTraceString.Write(
                "{0}.{1:X}.{2} ",
                entry.Epoch.DataLossNumber,
                entry.Epoch.ConfigurationNumber,
                entry.LastOperationLSN);
        }

        bool matchedProgressVector = false;
        for (auto ix = owner_->progressVector_.size() - 1; ix < numeric_limits<size_t>::max(); --ix)
        {
            ProgressVectorEntry const & entry = owner_->progressVector_[ix];

            if (copyContext.LastOperationLSN > entry.LastOperationLSN ||
                copyContext.Epoch.DataLossNumber != entry.Epoch.DataLossNumber ||
                copyContext.Epoch.ConfigurationNumber > entry.Epoch.ConfigurationNumber)
            {
                //
                // Secondary has made false progress
                //
                owner_->replicatedStore_.CopyStatistics.OnFalseProgress();

                WriteInfo(
                    TraceComponent,
                    "{0}: false progress ({1}): target=[{2}.{3:X}.{4}] entry=[{5}.{6:X}.{7}] vector=[{8}]",
                    owner_->TraceId,
                    copyContext.Id,
                    copyContext.Epoch.DataLossNumber,
                    copyContext.Epoch.ConfigurationNumber,
                    copyContext.LastOperationLSN,
                    entry.Epoch.DataLossNumber,
                    entry.Epoch.ConfigurationNumber,
                    entry.LastOperationLSN,
                    pvTrace);

                doFullCopy = true;
                copyStartSequenceNumber = 0;

                return ErrorCodeValue::Success;
            }
            else // if (copyContext.LastOperationLSN <= entry.LastOperationLSN)
            {
                if (copyContext.Epoch.ConfigurationNumber == entry.Epoch.ConfigurationNumber)
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}: matched configuration number ({1}): target=[{2}.{3:X}.{4}] entry=[{5}.{6:X}.{7}] vector=[{8}]",
                        owner_->TraceId,
                        copyContext.Id,
                        copyContext.Epoch.DataLossNumber,
                        copyContext.Epoch.ConfigurationNumber,
                        copyContext.LastOperationLSN,
                        entry.Epoch.DataLossNumber,
                        entry.Epoch.ConfigurationNumber,
                        entry.LastOperationLSN,
                        pvTrace);

                    doFullCopy = false;
                    matchedProgressVector = true;

                    // Start copying data items from secondary's last committed data
                    copyStartSequenceNumber = copyContext.LastOperationLSN + 1;

                    // Start epoch updates from secondary's current epoch
                    owner_->currentCopyProgressVectorIndex_ = static_cast<int>(ix);

                    // fall-through and perform further check against tombstone low watermark
                    //
                    break;
                }
                else
                {
                    // no-op: continue seeking backwards through progress vector
                }
            }
        } // for progress vector

        // Default to full copy when we can't match any conditions required for partial copy
        //
        if (!matchedProgressVector)
        {
            WriteInfo(
                TraceComponent,
                "{0}: epoch not found ({1}): target=[{2}.{3:X}.{4}] vector=[{5}]",
                owner_->TraceId,
                copyContext.Id,
                copyContext.Epoch.DataLossNumber,
                copyContext.Epoch.ConfigurationNumber,
                copyContext.LastOperationLSN,
                pvTrace);

            doFullCopy = true;
            copyStartSequenceNumber = 0;

            return ErrorCodeValue::Success;
        }

        // Check that the secondary is not too far behind the latest tombstone truncation,
        // which would prevent us from performing a partial copy for deleted entries.
        //
        TombstoneLowWatermarkData lowWatermark;
        error = this->GetLowWatermark(owner_->txSPtr_, lowWatermark);

        if (error.IsSuccess())
        {
            if (copyContext.LastOperationLSN < lowWatermark.OperationLSN)
            {
                //
                // Secondary has fallen too far behind tombstone truncation
                //
                auto count = owner_->replicatedStore_.CopyStatistics.OnStaleSecondary();

                WriteInfo(
                    TraceComponent,
                    "{0}: stale secondary ({1}) : target = [{2}.{3:X}.{4}] low watermark = {5} count = {6}",
                    owner_->TraceId,
                    copyContext.Id,
                    copyContext.Epoch.DataLossNumber,
                    copyContext.Epoch.ConfigurationNumber,
                    copyContext.LastOperationLSN,
                    lowWatermark.OperationLSN,
                    count);

                doFullCopy = true;
                copyStartSequenceNumber = 0;

                owner_->currentCopyProgressVectorIndex_ = -1;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: tombstone low watermark validated ({1}): target = [{2}.{3:X}.{4}] low watermark = {5}",
                    owner_->TraceId,
                    copyContext.Id,
                    copyContext.Epoch.DataLossNumber,
                    copyContext.Epoch.ConfigurationNumber,
                    copyContext.LastOperationLSN,
                    lowWatermark.OperationLSN);
            }
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            // Expected if tombstones have never been pruned
            error = ErrorCodeValue::Success;
        }

        return error;
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::GetNextCopyOperationForLocalApply(
        __out CopyOperation & result)
    {
        auto error = EnumerateCopyOperations(
            owner_->txSPtr_,
            owner_->lastEnumeratedOperationLSN_,
            CopyType::PagedCopy);

        if (error.IsSuccess())
        {
            result = move(copyOperation_);
        }

        return error;
    }

    void ComCopyOperationEnumerator::ComCopyOperationContext::ScheduleGetFileStreamFullCopyContext(AsyncOperationSPtr const & thisSPtr)
    {
        auto delay = StoreConfig::GetConfig().FileStreamFullCopyRetryDelay;

        WriteInfo(
            TraceComponent,
            "{0} retrying file stream full copy initialization in {1}",
            owner_->TraceId,
            delay);
                
        auto root = owner_->root_->CreateAsyncOperationRoot();
        Threadpool::Post(
            [this, thisSPtr, root]() { this->GetFileStreamFullCopyContext(thisSPtr); },
            delay);
    }

    void ComCopyOperationEnumerator::ComCopyOperationContext::GetFileStreamFullCopyContext(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_->replicatedStore_.FileStreamFullCopy->BeginGetFileStreamCopyContext(
            owner_->ActivityId,
            owner_->uptoOperationLSN_,
            [this](AsyncOperationSPtr const & operation) { this->OnGetFileStreamFullCopyContextComplete(operation, false); },
            thisSPtr);
        this->OnGetFileStreamFullCopyContextComplete(operation, true);
    }

    void ComCopyOperationEnumerator::ComCopyOperationContext::OnGetFileStreamFullCopyContextComplete(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        auto error = owner_->replicatedStore_.FileStreamFullCopy->EndGetFileStreamCopyContext(
            operation,
            owner_->fileStreamFullCopyContextSPtr_);

        if (error.IsSuccess())
        {
            error = this->ReadNextFileStreamChunk(true); // isFirstChunk
        }
        else if (error.IsError(ErrorCodeValue::MaxFileStreamFullCopyWaiters))
        {
            // We have reached the maximum number of allowed waiters (configurable)
            // for file stream full copy. Default back to logical full copy in
            // order to perform more full copies in parallel (used in random single-process tests).
            //
            error = owner_->replicatedStore_.CreateLocalStoreTransaction(owner_->txSPtr_);

            if (error.IsSuccess())
            {
                owner_->isFileStreamFullCopy_ = false;

                error = EnumerateCopyOperations(
                    owner_->txSPtr_,
                    0, // copyStartSequenceNumber
                    CopyType::FirstFullCopy);
            }
        }
        else if (error.IsError(ErrorCodeValue::BackupInProgress))
        {
            this->ScheduleGetFileStreamFullCopyContext(thisSPtr);

            return;
        }
        else
        {
            owner_->isFirstGetNext_ = false;
        }

        this->TryComplete(thisSPtr, error);
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::ReadNextFileStreamChunk(bool isFirstChunk)
    {
        if (!owner_->fileStreamFullCopyContextSPtr_)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: FileStreamFullCopyContext is null", owner_->TraceId);

            return ErrorCodeValue::Success;
        }

        unique_ptr<FileStreamCopyOperationData> fileStreamDataUPtr;
        auto error = owner_->fileStreamFullCopyContextSPtr_->ReadNextFileStreamChunk(
            isFirstChunk,
            owner_->replicatedStore_.TargetCopyOperationSize,
            fileStreamDataUPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        if (fileStreamDataUPtr)
        {
            auto copyType = owner_->replicatedStore_.Settings.FullCopyMode;
            bool isRebuildMode = (copyType == FullCopyMode::Rebuild);

            if (isFirstChunk)
            {
                WriteInfo(
                    TraceComponent,
                    "{0} file stream full copy mode: {1}",
                    owner_->TraceId,
                    copyType);

                if (isRebuildMode)
                {
                    owner_->replicatedStore_.CopyStatistics.OnFileStreamRebuildFullCopy();
                }
                else
                {
                    owner_->replicatedStore_.CopyStatistics.OnFileStreamFullCopy();
                }
            }

            copyOperation_ = CopyOperation(move(fileStreamDataUPtr), isRebuildMode);
        }
        else
        {
            error = owner_->replicatedStore_.FileStreamFullCopy->ReleaseFileStreamCopyContext(
                owner_->ActivityId,
                move(owner_->fileStreamFullCopyContextSPtr_));
        }

        return error;
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::EnumerateCopyOperations(
        ILocalStore::TransactionSPtr const & txSPtr,
        ::FABRIC_SEQUENCE_NUMBER enumerationStartOperationLSN,
        CopyType::Enum copyType)
    {
        if (!txSPtr)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0}: EnumerateCopyOperations(): null transaction", owner_->TraceId);

            return ErrorCodeValue::ArgumentNull;
        }

        // *******************************************************************************************************************************
        // Data items can be split into copy operations in the following ways:
        //
        // 1) if the current copy page is atomically consistent
        // 2) on sequence number boundaries
        // 3) when the cumulative estimated byte size of the copy operation exceeds copyOperationApproximateSize_ while reading
        //
        // 1) takes precedence over 2) and 3), which means that we cannot split the copy stream at a point that would split
        // an atomic transaction. For example, given the following transactions:
        //
        // LSN  |  Key A  | Key B | Key C
        // ------------------------------
        // 1        a1       b1      c1
        // 2        a2       b2
        // 3        a3               c3
        //
        // We cannot page after LSN 2 because we no longer have values a2 and c1. The stream isn't atomically consistent until LSN 3.
        //
        // If MaxCopyOperationSize is hit before we can find a consistent splitting point, then we fall back on having the secondary
        // perform partial build on a snapshot database on the side.
        //
        // 2) takes precedence over 3), which means that once we start reading a series of data items with the same sequence number, we
        // will read all data items with that sequence number into the current copy operation. If the estimated byte size has not
        // exceeded the defined limit, we will move on to reading the next series of data items with the same sequence number.
        // This implies that a copy operation will never be smaller than the original replication operation.
        //
        // This simplifies retrieving copy operations across begin/end boundaries using the sequence number, since EseLocalStore
        // supports enumerating in either sequence number order or key name order.
        //
        // In order to track 1) we needed to insert metadata entries about which keys were modified in each transaction.
        // Unfortunately, we found that writing these additional metadata rows had performance consequences inversely
        // proportional to the number of rows modified in each transaction (i.e. the effects were most significant
        // with single row operations). To avoid this performance degradation in stable state, we instead
        // perform incremental copy on a snapshot database if any paging is required. The only time incremental
        // copy will occur on the existing database is if the complete incremental copy fits in a single copy page.
        //
        // *******************************************************************************************************************************

        bool isCopyStreamFinished = false;
        size_t approximateSize = 0;
        vector<ReplicationOperation> replicationOperations;

        ILocalStore::EnumerationSPtr enumSPtr;
        ErrorCode error = owner_->LocalStore->CreateEnumerationByOperationLSN(txSPtr, enumerationStartOperationLSN, enumSPtr);

        if (!error.IsSuccess())
        {
            return error;
        }

        ::FABRIC_SEQUENCE_NUMBER currentOperationLSN = enumerationStartOperationLSN;
        ::FABRIC_SEQUENCE_NUMBER seriesOperationLSN = 0;

        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring type;
            if (!(error = enumSPtr->CurrentType(type)).IsSuccess()) { return error; }

            if (type == Constants::ProgressDataType || type == Constants::LocalStoreIncrementalBackupDataType)
            {
                // Some ReplicatedStore metadata is handled separately since it
                // is not part of the "user data" copy stream
                //
                continue;
            }

            if (!(error = enumSPtr->CurrentOperationLSN(currentOperationLSN)).IsSuccess()) { return error; }

            if (currentOperationLSN > owner_->uptoOperationLSN_)
            {
                break;
            }

            if (seriesOperationLSN == 0)
            {
                // Initialize paging information for first actual LSN found
                //
                seriesOperationLSN = currentOperationLSN;
            }

            wstring key;
            vector<byte> bytes;
			FILETIME lastModifiedOnPrimaryUtc;

            if (!(error = enumSPtr->CurrentKey(key)).IsSuccess()) { return error; }
            if (!(error = enumSPtr->CurrentValue(bytes)).IsSuccess()) { return error; }
			if (!(error = enumSPtr->CurrentLastModifiedOnPrimaryFILETIME(lastModifiedOnPrimaryUtc)).IsSuccess()) { return error; }

            // Don't try to get a strict upper bound on how much
            // memory is needed to buffer each copy operation since
            // it's difficult to predict the exact final size
            // after serialization
            //
            approximateSize += (type.size() * sizeof(wchar_t));
            approximateSize += (key.size() * sizeof(wchar_t));
            approximateSize += bytes.size();

            approximateSize += sizeof(ReplicationOperationType::Enum);
            approximateSize += sizeof(_int64);
            approximateSize += sizeof(FABRIC_SEQUENCE_NUMBER);
			approximateSize += sizeof(DateTime); // ReplicationOperation internally store FILETIME as DateTime for serialization.

            // The first data item in the next series of data items may
            // need to transfer as part of the next copy operation.
            // Check the size of this copy operation so far.
            //
            if (currentOperationLSN != seriesOperationLSN)
            {
                bool shouldPage = (approximateSize > owner_->replicatedStore_.TargetCopyOperationSize);

                if (shouldPage)
                {
                    // To preserve transactional consistency of incomplete partial (incremental) builds,
                    // we must either complete the incremental build in a single transaction (single copy page)
                    // or perform the incremental build on a snapshot of the secondary database - only
                    // attaching to the database once the entire incremental build completes.
                    //
                    if (copyType == CopyType::FirstPartialCopy)
                    {
                        WriteInfo(
                            TraceComponent,
                            "{0}: setting FirstPartialCopy flag on copy page at LSN: current={1} series={2}",
                            owner_->TraceId,
                            currentOperationLSN,
                            seriesOperationLSN);

                        copyType = CopyType::FirstSnapshotPartialCopy;
                    }

                    // Defer the next sequence number series of data items
                    // to the next copy operation
                    //
                    break;
                }
                else
                {
                    // Include the next sequence number series of data items
                    // in this copy operation
                    //
                    seriesOperationLSN = currentOperationLSN;
                }
            }

            replicationOperations.push_back(ReplicationOperation(
                ReplicationOperationType::Copy,
                type,
                key,
                bytes.data(),
                bytes.size(),
                currentOperationLSN,
                lastModifiedOnPrimaryUtc));

        } // end data item enumeration loop

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: enumerated copy operations: last enumerated operationLSN = {1} requested = {2} count = {3}",
                owner_->TraceId,
                currentOperationLSN,
                owner_->uptoOperationLSN_,
                replicationOperations.size());

            if (currentOperationLSN > owner_->uptoOperationLSN_)
            {
                isCopyStreamFinished = true;
            }
        }
        else if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            if (replicationOperations.size() > 0)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: completed enumerating copy operations: last enumerated operationLSN = {1} requested = {2} count = {3}",
                    owner_->TraceId,
                    currentOperationLSN,
                    owner_->uptoOperationLSN_,
                    replicationOperations.size());

                // Finished enumeration. Increment currentOperationLSN so that
                // the subsequent call to GetNext() does not re-read the last
                // chunk of data items.
                //
                ++currentOperationLSN;
            }

            isCopyStreamFinished = true;
            error.Reset(ErrorCodeValue::Success);
        }
        else
        {
            // Error reading from database - fail this operation.
            // Do not set lastEnumeratedOperationLSN_ so that the caller can
            // call GetNext() again and we will start enumerating
            // at the sequence number just after the last successfully
            // copied sequence number.
            //
            return error;
        }

        // *******************************************************************************
        // There are currently three special data items inserted into the copy stream:
        //
        //   1) "Current" epoch updates
        //   2) Epoch history
        //   3) Tombstone low watermark
        //
        // Epoch updates are persisted by the secondary as its new "current" epoch. This
        // allows the secondary to continue partial copy using a different primary if
        // this primary crashes before the copy process completes. Epoch updates are added
        // to the end of a copy operation if data in the copy operation crosses an epoch boundary.
        //
        // Epoch history is the last item sent in the copy stream and overwrites the
        // secondary's previous epoch history if it already exists. We cannot simply
        // build the epoch history incrementally on the secondary since the primary may
        // not send every epoch in its history as an update. For example, a single copy operation
        // may span multiple epochs, in which case only the latest epoch is sent as a
        // an epoch update.
        //
        // If found, the tombstone low watermark is also copied over to the secondary and
        // updated. The secondary cannot rely on its own locally generated low watermark
        // since it may have to take over as primary and build other secondaries without
        // ever performing a tombstone prune itself.
        // *******************************************************************************

        bool addedCurrentEpochUpdate = false;
        ReplicationOperation currentEpochUpdate;

        if (error.IsSuccess())
        {
            if (this->TryGetCurrentEpochUpdate(
                currentOperationLSN > 0 ? currentOperationLSN - 1 : 0,
                currentEpochUpdate,
                error)) // out
            {
                addedCurrentEpochUpdate = true;
                replicationOperations.push_back(std::move(currentEpochUpdate));
            }
        }

        if (error.IsSuccess() && isCopyStreamFinished)
        {
            ReplicationOperation epochHistoryUpdate;
            if (this->TryGetEpochHistoryUpdate(
                txSPtr,
                epochHistoryUpdate,
                error)) // out
            {
                replicationOperations.push_back(std::move(epochHistoryUpdate));

                if (!addedCurrentEpochUpdate)
                {
                    error = this->GetCurrentEpochUpdate(
                        owner_->progressVector_.size() - 1,
                        currentEpochUpdate);

                    replicationOperations.push_back(std::move(currentEpochUpdate));
                }
            }

            if (error.IsSuccess() && owner_->isLogicalFullCopy_)
            {
                ReplicationOperation lowWatermarkUpdate;
                if (this->TryGetLowWatermarkUpdate(
                    txSPtr,
                    lowWatermarkUpdate,
                    error)) // out
                {
                    replicationOperations.push_back(std::move(lowWatermarkUpdate));
                }
            }
        }

        if (error.IsSuccess())
        {
            copyOperation_ = CopyOperation(copyType, move(replicationOperations));

            owner_->lastEnumeratedOperationLSN_ = currentOperationLSN;
        }

        return error;
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::CreateComReplicatedOperationData(__out ComPointer<ComReplicatedOperationData> & result)
    {
        ErrorCode error;
        Serialization::IFabricSerializableStreamUPtr serializedStreamUPtr;

        if (copyOperation_.IsEmpty)
        {
            // Release inner store transaction as soon as we complete enumeration
            // for two reasons:
            //
            // 1) Decouples from member destruction order (destructing
            //    ComponentRootSPtr will destruct EseLocalStore, which
            //    will in turn wait to drain pending ESE transactions).
            //
            // 2) The secondary may perform additional work (OnCopyComplete)
            //    even after pumping and applying all copy operations, which
            //    may take time. Releasing the transaction releases underlying
            //    DB resources (e.g. ESE version pages).
            //
            owner_->txSPtr_.reset();

            // Also release tombstone low watermark after all operations have
            // been read from the primary database. Do not wait for the secondary
            // to finish applying and the overall copy to complete.
            //
            owner_->ReleaseLowWatermarkReaderCount();
        }
        else
        {
            error = FabricSerializer::Serialize(&copyOperation_, serializedStreamUPtr);
        }

        if (error.IsSuccess())
        {
            result = make_com<ComReplicatedOperationData>(Ktl::Move(serializedStreamUPtr));

            stopwatch_.Stop();

            owner_->replicatedStore_.PerfCounters.AvgLatencyOfCopyCreateBase.Increment();
            owner_->replicatedStore_.PerfCounters.AvgLatencyOfCopyCreate.IncrementBy(stopwatch_.ElapsedMilliseconds);

            owner_->replicatedStore_.PerfCounters.AvgSizeOfCopyBase.Increment();
            owner_->replicatedStore_.PerfCounters.AvgSizeOfCopy.IncrementBy(result->OperationDataSize);
        }

        return error;
    }

    HRESULT STDMETHODCALLTYPE ComCopyOperationEnumerator::ComCopyOperationContext::End(__out ComPointer<ComReplicatedOperationData> & result)
    {
        HRESULT hr = ComAsyncOperationContext::End();
        if (!SUCCEEDED(hr)) { return hr; }

        return this->CreateComReplicatedOperationData(result).ToHResult();
    }

    int ComCopyOperationEnumerator::ComCopyOperationContext::GetCopyEpochIndex(::FABRIC_SEQUENCE_NUMBER lastEnumeratedOperationLSN)
    {
        size_t ix = 0;
        if (owner_->currentCopyProgressVectorIndex_ > 0)
        {
            ix = owner_->currentCopyProgressVectorIndex_;
        }

        // Find only the latest epoch to send as an update
        while (ix < owner_->progressVector_.size())
        {
            if (owner_->progressVector_[ix].LastOperationLSN > lastEnumeratedOperationLSN)
            {
                break;
            }

            ++ix;
        }

        if (ix >= owner_->progressVector_.size())
        {
            ix = owner_->progressVector_.size() - 1;
        }

        return static_cast<int>(ix);
    }

    bool ComCopyOperationEnumerator::ComCopyOperationContext::TryGetCurrentEpochUpdate(
        ::FABRIC_SEQUENCE_NUMBER lastEnumeratedOperationLSN,
        __out ReplicationOperation & result,
        __out ErrorCode & error)
    {
#if defined(PLATFORM_UNIX)
        if (owner_->epochHistorySent_)
        {
            return false;
        }
#endif
        int lastEnumeratedCopyEpochIndex = GetCopyEpochIndex(lastEnumeratedOperationLSN);
        if (lastEnumeratedCopyEpochIndex > owner_->currentCopyProgressVectorIndex_)
        {
            owner_->currentCopyProgressVectorIndex_ = lastEnumeratedCopyEpochIndex;

            error = this->GetCurrentEpochUpdate(lastEnumeratedCopyEpochIndex, result);

            return error.IsSuccess();
        }
        else
        {
            int currentIndex = owner_->currentCopyProgressVectorIndex_;
            WriteNoise(
                TraceComponent,
                "{0}: skipping epoch update: last enumerated index = {1} current index = {2} current epoch = {3}.{4:X}",
                owner_->TraceId,
                lastEnumeratedCopyEpochIndex,
                currentIndex,
                owner_->progressVector_[currentIndex].Epoch.DataLossNumber,
                owner_->progressVector_[currentIndex].Epoch.ConfigurationNumber);

            error = ErrorCodeValue::Success;
            return false;
        }
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::GetCurrentEpochUpdate(
        size_t progressVectorIndex,
        __out ReplicationOperation & result)
    {
        CurrentEpochData currentEpoch(owner_->progressVector_[progressVectorIndex].Epoch);

        KBuffer::SPtr bufferSPtr;
        auto error = FabricSerializer::Serialize(&currentEpoch, bufferSPtr);
        if (!error.IsSuccess())
        {
            return error;
        }

        WriteNoise(
            TraceComponent,
            "{0}: adding current epoch update: epoch = {1}.{2:X} high watermark = {3}",
            owner_->TraceId,
            owner_->progressVector_[progressVectorIndex].Epoch.DataLossNumber,
            owner_->progressVector_[progressVectorIndex].Epoch.ConfigurationNumber,
            owner_->progressVector_[progressVectorIndex].LastOperationLSN);

        result = ReplicationOperation(
            ReplicationOperationType::Copy,
            Constants::ProgressDataType,
            Constants::CurrentEpochDataName,
            *bufferSPtr);

        return error;
    }

    bool ComCopyOperationEnumerator::ComCopyOperationContext::TryGetEpochHistoryUpdate(
        ILocalStore::TransactionSPtr const & txSPtr,
        __out ReplicationOperation & result,
        __out ErrorCode & error)
    {
        if (!owner_->epochHistorySent_)
        {
            owner_->epochHistorySent_ = true;

            auto serializer = make_unique<Store::DataSerializer>(
                owner_->replicatedStore_.PartitionedReplicaId, 
                *owner_->localStoreUPtr_);

            ProgressVectorData epochHistory;
            error = serializer->TryReadData(
                txSPtr,
                Constants::ProgressDataType,
                Constants::EpochHistoryDataName,
                epochHistory);

            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: could not read epoch history for copy: error = {1}",
                    owner_->TraceId,
                    error);

                return false;
            }
            else
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: read epoch history for copy: {1}",
                    owner_->TraceId,
                    epochHistory);
            }

            KBuffer::SPtr bufferSPtr;
            error = FabricSerializer::Serialize(&epochHistory, bufferSPtr);

            if (!error.IsSuccess())
            {
                return false;
            }

            WriteNoise(
                TraceComponent,
                "{0}: adding epoch history transfer: history count = {1}",
                owner_->TraceId,
                epochHistory.ProgressVector.size());

            result = ReplicationOperation(
                ReplicationOperationType::Copy,
                Constants::ProgressDataType,
                Constants::EpochHistoryDataName,
                *bufferSPtr);

            return true;
        }
        else
        {
            error = ErrorCodeValue::Success;
            return false;
        }
    }

    ErrorCode ComCopyOperationEnumerator::ComCopyOperationContext::GetLowWatermark(
        ILocalStore::TransactionSPtr const & txSPtr,
        __out TombstoneLowWatermarkData & result)
    {
        return owner_->replicatedStore_.GetTombstoneLowWatermark(txSPtr, result);
    }

    bool ComCopyOperationEnumerator::ComCopyOperationContext::TryGetLowWatermarkUpdate(
        ILocalStore::TransactionSPtr const & txSPtr,
        __out ReplicationOperation & result,
        __out ErrorCode & error)
    {
        if (owner_->lowWatermarkSent_)
        {
            error = ErrorCodeValue::Success;
            return false;
        }

        owner_->lowWatermarkSent_ = true;

        TombstoneLowWatermarkData lowWatermark;
        error = this->GetLowWatermark(txSPtr, lowWatermark);

        if (error.IsSuccess())
        {
            KBuffer::SPtr bufferSPtr;
            error = FabricSerializer::Serialize(&lowWatermark, bufferSPtr);

            if (error.IsSuccess())
            {
                result = ReplicationOperation(
                    ReplicationOperationType::Copy,
                    Constants::ProgressDataType,
                    Constants::TombstoneLowWatermarkDataName,
                    *bufferSPtr);

                return true;
            }
        }
        else if (error.IsError(ErrorCodeValue::NotFound))
        {
            error = ErrorCodeValue::Success;
            return false;
        }

        return false;
    }
}

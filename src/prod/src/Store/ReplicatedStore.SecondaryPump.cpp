// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TestHooks;

namespace Store
{

    StringLiteral const TraceComponent("SecondaryPump");

    //
    // PendingInsertMetadata
    //

    class ReplicatedStore::SecondaryPump::PendingInsertMetadata
    {
    private:
        PendingInsertMetadata(wstring const & type, wstring const & key, FABRIC_SEQUENCE_NUMBER lsn)
            : type_(type)
            , key_(key)
            , lsn_(lsn)
        {
        }

    public:

        static shared_ptr<PendingInsertMetadata> Create(wstring const & type, wstring const & key, FABRIC_SEQUENCE_NUMBER lsn)
        {
            return shared_ptr<PendingInsertMetadata>(new PendingInsertMetadata(type, key, lsn));
        }

        __declspec(property(get=get_Type)) wstring const & Type;
        __declspec(property(get=get_Key)) wstring const & Key;
        __declspec(property(get=get_Lsn)) ::FABRIC_SEQUENCE_NUMBER Lsn;

        wstring const & get_Type() const { return type_; }
        wstring const & get_Key() const { return key_; }
        ::FABRIC_SEQUENCE_NUMBER get_Lsn() const { return lsn_; }

        static wstring GetHash(wstring const & type, wstring const & key)
        {
            return wformatString("{0}:{1}", type, key);
        }

        wstring GetHash()
        {
            return GetHash(type_, key_);
        }

    private:
        wstring type_;
        wstring key_;
        FABRIC_SEQUENCE_NUMBER lsn_;
    };

    //
    // SecondaryPump
    //

    ReplicatedStore::SecondaryPump::SecondaryPump(
        __in ReplicatedStore & replicatedStore,
        ComPointer<IFabricStateReplicator> const & stateReplicator)
        : PartitionedReplicaTraceComponent(replicatedStore.PartitionedReplicaId)
        , root_(replicatedStore.Root.shared_from_this())
        , replicatedStore_(replicatedStore)
        , copyLocalStore_()
        , isFullCopy_(false)
        , isLogicalCopyStreamComplete_(false)
        , shouldNotifyCopyComplete_(false)
        , lastCopyLsn_(0)
        , operationStream_()
        , stateReplicator_(stateReplicator)
        , pumpState_(PumpNotStarted)
        , stopwatch_()
        , lastLsnProcessed_(0)
        , lsnAtLastSyncCommit_(0)
        , thisLock_()
        , isCanceled_(false)
        , isStreamFaulted_(false)
        , pendingOperationsCount_(1) // Start with count of 1 to control lifetime
    {
        WriteNoise(
            TraceComponent, 
            "{0} ctor: this = {1}",
            this->TraceId,
            static_cast<void*>(this));
    }

    ReplicatedStore::SecondaryPump::~SecondaryPump()
    {
        WriteNoise(
            TraceComponent, 
            "{0} ~dtor: this = {1}",
            this->TraceId,
            static_cast<void*>(this));
    }

    ReplicatedStore::SecondaryPump::SecondaryPumpState ReplicatedStore::SecondaryPump::get_PumpState()
    {
        AcquireReadLock lock(thisLock_);
        return pumpState_;
    }

    ILocalStoreUPtr const & ReplicatedStore::SecondaryPump::GetCurrentLocalStore()
    {
        return (copyLocalStore_ ? copyLocalStore_ : replicatedStore_.LocalStore);
    }

    void ReplicatedStore::SecondaryPump::Start()
    {
        WriteInfo(TraceComponent, "{0} starting secondary pump.", this->TraceId);

        replicatedStore_.Notifications->StartDispatchingNotifications();

        this->ScheduleDrainOperations();
    }

    void ReplicatedStore::SecondaryPump::ScheduleDrainOperations()
    {
        auto selfRoot = this->CreateComponentRoot();
        Threadpool::Post([this, selfRoot]() { this->DrainOperations(); });
    }

    void ReplicatedStore::SecondaryPump::DrainOperations()
    {
        if (this->ApproveNewOperationCreation())
        {
            // Drain replication queue synchronously
            AsyncOperationSPtr pumpOperation;
            do
            {
                pumpOperation = this->PumpOperation();

            } while (pumpOperation && pumpOperation->CompletedSynchronously);

            // Once the replication queue is drained, create a dummy transaction and commit it.
            // This causes ESE to flush changes from pending async commits to disk.
            //
            if (replicatedStore_.Settings.EnableFlushOnDrain && pumpOperation)
            {
                this->FlushCurrentLocalStore();
            }

            this->OnPendingOperationCompletion();
        }
    }

    void ReplicatedStore::SecondaryPump::FlushCurrentLocalStore()
    {
        if (fullCopyFileUPtr_)
        {
            // Creating database archive directly from file stream.
            // No transaction flushing is needed.
            //
            return;
        }

        TransactionSPtr txSPtr;
        ErrorCode error;

        if (copyLocalStore_)
        {
            error = copyLocalStore_->CreateTransaction(txSPtr);
        }
        else
        {
            // Acquire the inner store lock to prevent race condition with
            // deletion of database directory
            //
            error = replicatedStore_.CreateLocalStoreTransaction(txSPtr);
        }

        if (error.IsSuccess())
        {
            error = txSPtr->Commit();
        }

        if (error.IsSuccess())
        {
            ReplicatedStoreEventSource::Trace->SecondaryDummyCommit(
                this->PartitionedReplicaId,
                lastLsnProcessed_,
                lsnAtLastSyncCommit_);

            lsnAtLastSyncCommit_ = lastLsnProcessed_;
        }
        else if (error.IsError(ErrorCodeValue::StoreFatalError))
        {
            this->TransientFaultReplica(error, L"Failed to commit secondary dummy transaction", true /*scheduleDrain*/);
        }
    }

    AsyncOperationSPtr ReplicatedStore::SecondaryPump::PumpOperation()
    {
        ReplicatedStoreEventSource::Trace->SecondaryPumpOperation(this->PartitionedReplicaId);
        ComPointer<IFabricOperationStream> stream;
        {
            AcquireReadLock lock(thisLock_);

            if (isCanceled_)
            {
                WriteInfo(TraceComponent, "{0} canceling PumpOperation()", this->TraceId);

                return nullptr;
            }

            stream = operationStream_;
        }

        replicatedStore_.Test_BlockSecondaryPumpIfNeeded();

        // Initialize the copy stream from the replicator if needed;
        // Once the last empty copy operation is pumped, switch to the replication stream.
        // If close is called before the last copy operation is received,
        // do not start the replication pump.
        //
        if (get_PumpState() == PumpNotStarted && !stream)
        {
            HRESULT hr;
            {
                AcquireWriteLock lock(thisLock_);
                hr = stateReplicator_->GetCopyStream(operationStream_.InitializationAddress());
                stream = operationStream_;

                if (SUCCEEDED(hr))
                {
                    pumpState_ = PumpCopy;
                }
            }

            if (!SUCCEEDED(hr))
            {
                this->TransientFaultReplica(ErrorCode::FromHResult(hr), L"GetCopyStream failed", false);

                return nullptr;
            }
        }

        auto pumpOperation = AsyncOperation::CreateAndStart<ComPumpAsyncOperation>(
            stream,
            [this](AsyncOperationSPtr const & pumpOperation) { this->OnPumpOperationComplete(pumpOperation, false); },
            this->CreateAsyncOperationRoot());
        this->OnPumpOperationComplete(pumpOperation, true);

        return pumpOperation;
    }

    void ReplicatedStore::SecondaryPump::OnPumpOperationComplete(
        AsyncOperationSPtr const & pumpOperation, 
        bool expectedCompletedSynchronously)
    {
        if (!pumpOperation->CompletedSynchronously == expectedCompletedSynchronously) { return; }

        replicatedStore_.PerfCounters.RateOfPump.Increment();

        stopwatch_.Restart();

        // Drain loop will continue as long the pump completes synchronously
        //
        bool scheduleDrainOnFault = !pumpOperation->CompletedSynchronously;

        ComPointer<IFabricOperation> operationCPtr;
        auto error = ComPumpAsyncOperation::End(
            pumpOperation, 
            operationCPtr);

        if (!error.IsSuccess())
        {
            auto message = wformatString(
                "OnPumpOperationComplete(): synchronous={0}",
                pumpOperation->CompletedSynchronously);

            this->TransientFaultReplica(error, message, scheduleDrainOnFault);

            return;
        }

        wstring errorMessage;
        auto pumpState = this->get_PumpState();

        if (!operationCPtr  || operationCPtr->get_Metadata()->Type == FABRIC_OPERATION_TYPE_END_OF_STREAM)
        {
            switch (pumpState)
            {
            case PumpCopy:
                //
                // Handles end of stream processing for partial copy and 
                // legacy (not file stream) full copy 
                //
                error = this->ProcessEndOfCopyStream(pumpOperation, operationCPtr, errorMessage);

                if (error.IsSuccess())
                {
                    // Restart pump on replication stream
                    //
                    this->RestartDrainOperationsIfNeeded(pumpOperation);
                }

                break;

            case PumpReplication:
            default:
                //
                // Replication engine has shutdown or switched out of the secondary role
                //
                error = this->ProcessEndOfReplicationStream(pumpOperation, operationCPtr, errorMessage);

                break;

            } // switch pumpState

            if (!error.IsSuccess())
            {
                this->TransientFaultReplica(error, errorMessage, scheduleDrainOnFault);
            }

            // Nothing to apply in all cases
            //
            return;

        } // end of copy/replication stream

        if (isStreamFaulted_.load())
        {
            // Drain and drop remaining copy/replication operations once the stream is faulted
            //
            this->RestartDrainOperationsIfNeeded(pumpOperation);

            return;
        }
        
        //
        // Having successfully pumped an operation, we must be able to apply
        // it successfully. So any failures while applying replicated operations 
        // after this point must cause the secondary to go down.
        //

        ::FABRIC_OPERATION_METADATA const * operationMetadata = operationCPtr->get_Metadata();
        ::FABRIC_SEQUENCE_NUMBER operationLsn = operationMetadata->SequenceNumber;

        auto replicationOperations = make_shared<vector<ReplicationOperation>>();
        ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked = 0;

        switch (pumpState)
        {
        case PumpCopy:
            error = this->ProcessCopyOperation(
                pumpOperation,
                operationCPtr,
                operationLsn,
                *replicationOperations,
                lastQuorumAcked,
                errorMessage);

            break;

        case PumpReplication:
            error = this->ProcessReplicationOperation(
                pumpOperation,
                operationCPtr,
                operationLsn,
                *replicationOperations,
                lastQuorumAcked,
                errorMessage);

            break;

        default:
            errorMessage = wformatString("invalid pump state {0}", static_cast<int>(pumpState)); 

            error = ErrorCodeValue::InvalidOperation;

            break;

        } // switch pumpState

        if (error.IsSuccess() && !replicationOperations->empty())
        {
            // Operations are applied serially on the secondary, so there is only one active transaction
            // at a time.
            //
            error = this->ApplyOperationsWithRetry(
                operationCPtr, 
                replicationOperations, 
                operationLsn, 
                lastQuorumAcked, 
                errorMessage);

            if (pumpState == PumpCopy)
            {
                replicatedStore_.PerfCounters.AvgLatencyOfApplyCopyBase.Increment();
                replicatedStore_.PerfCounters.AvgLatencyOfApplyCopy.IncrementBy(stopwatch_.ElapsedMilliseconds);
            }
            else
            {
                replicatedStore_.PerfCounters.AvgLatencyOfApplyReplicationBase.Increment();
                replicatedStore_.PerfCounters.AvgLatencyOfApplyReplication.IncrementBy(stopwatch_.ElapsedMilliseconds);
            }
        }

        if (error.IsSuccess())
        {
            this->RestartDrainOperationsIfNeeded(pumpOperation);
        }
        else
        {
            this->TransientFaultReplica(error, errorMessage, scheduleDrainOnFault);
        }
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessEndOfCopyStream(
        AsyncOperationSPtr const & pumpOperation,
        ComPointer<IFabricOperation> const & operationCPtr,
        __out wstring & errorMessage)
    {

        auto error = this->InnerProcessEndOfCopyStream(pumpOperation, operationCPtr, errorMessage);

        // Copy completion (FinishFullCopy, NotifyCopyComplete, etc.) may take a long time.
        // By holding the copy EOS ack until after these operations complete, we prevent
        // the build from completing, which prevents the FM from attempting to swap the
        // primary back to this secondary before it's ready during upgrade.
        //
        // This improves service uptime by allowing the old primary to keep its primary
        // status until this secondary is truly ready (idle->active).
        //
        // UseStreamFaultsAndEndOfStreamOperationAck must be enabled on the replicator
        // settings to support this optimization.
        //
        // Always ack EOS regardless of whether EOS processing succeeded or replicator
        // will block any subsequent transient fault.
        //
        if (operationCPtr)
        {
            HRESULT hr = operationCPtr->Acknowledge();

            auto ackError = ErrorCode::FromHResult(hr);

            if (!ackError.IsSuccess())
            {
                if (error.IsSuccess())
                {
                    error = ackError;

                    errorMessage = wformatString("Acknowledge() end of copy stream failed: hr={0}", hr);
                }
                else
                {
                    WriteWarning(
                        TraceComponent,
                        "{0} failed to Ack copy EOS: hr={1}",
                        this->TraceId,
                        hr);
                }
            }
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::InnerProcessEndOfCopyStream(
        AsyncOperationSPtr const & pumpOperation,
        ComPointer<IFabricOperation> const & operationCPtr,
        __out wstring & errorMessage)
    {
        errorMessage.clear();

        WriteInfo(
            TraceComponent,
            "{0} pumped {1} operation from copy stream: sync={2}",
            this->TraceId,
            operationCPtr ? L"EOS" : L"NULL",
            pumpOperation->CompletedSynchronously);

        HRESULT hr = S_OK;
        {
            // The last NULL copy operation is received, so switch to replication stream
            //
            AcquireWriteLock lock(thisLock_);

            pumpState_ = PumpReplication;
            operationStream_.SetNoAddRef(NULL);
            hr = stateReplicator_->GetReplicationStream(operationStream_.InitializationAddress());
        }

        if (!SUCCEEDED(hr)) 
        { 
            errorMessage = L"GetReplicationStream() failed";

            return ErrorCode::FromHResult(hr); 
        }

        if (isStreamFaulted_.load())
        {
            // Return here since we still need to switch and pump the
            // rest of the faulted replication stream
            //
            return ErrorCodeValue::Success;
        }

#if defined(PLATFORM_UNIX)
        if (copyLocalStore_ != nullptr)
        {
            copyLocalStore_->Flush();
        }
#endif
        // Ensure that the copy database is only swapped if we know the stream is
        // complete at the protocol level. Don't depend on pumping a null copy
        // operation, which can happen on close when the copy is interrupted.
        //
        if (isLogicalCopyStreamComplete_)
        {
            auto error = isFullCopy_
                ? replicatedStore_.FinishFullCopy(move(copyLocalStore_))
                : replicatedStore_.FinishPartialCopy(*this, move(copyLocalStore_));

            copyLocalStore_.reset();

            if (!error.IsSuccess())
            {
                errorMessage = wformatString("failed to finish {0} copy", isFullCopy_ ? "full" : "partial");

                return error;
            }
        }

#if defined(PLATFORM_UNIX)
        if (copyLocalStore_ != nullptr)
        {
            copyLocalStore_->Flush();
        }

        if (replicatedStore_.LocalStore != nullptr)
        {
            replicatedStore_.LocalStore->Flush();
        }
#endif

        if (shouldNotifyCopyComplete_)
        {
            auto error = replicatedStore_.RecoverEpochHistory();

            if (!error.IsSuccess())
            {
                errorMessage = L"RecoverEpochHistory() failed";

                return error;
            }

            // Blocks on user processing of copy completion. This callback is guaranteed to occur
            // before secondary->primary change role completes, but not before *->secondary change
            // role completes.
            //
            error = replicatedStore_.Notifications->NotifyCopyComplete();

            if (!error.IsSuccess())
            {
                errorMessage = L"NotifyCopyComplete() failed";

                return error;
            }

            // Tombstones are generated locally during replication, so there are no compatibility issues
            // when the primary/secondary are configured with different tombstone versions. However,
            // tombstones must be migrated to the correct local format after copy since they may have
            // come from a replica configured using a different version.
            //
            error = replicatedStore_.RecoverTombstones();

            if (!error.IsSuccess())
            {
                errorMessage = L"RecoverTombstones() failed";

                return error;
            }
        }
        else
        {
            WriteInfo(
                TraceComponent,
                "{0} skipping copy completion notification and tombstone recovery",
                this->TraceId);
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessEndOfReplicationStream(
        AsyncOperationSPtr const & pumpOperation,
        ComPointer<IFabricOperation> const & operationCPtr,
        __out wstring & errorMessage)
    {
        WriteInfo(
            TraceComponent,
            "{0} pumped {1} operation from replication stream: sync={2}",
            this->TraceId,
            operationCPtr ? L"EOS" : L"NULL",
            pumpOperation->CompletedSynchronously);

#if defined(PLATFORM_UNIX)
        if (replicatedStore_.LocalStore != nullptr)
        {
            replicatedStore_.LocalStore->Flush();
        }
#endif

        if (operationCPtr)
        {
            HRESULT hr = operationCPtr->Acknowledge();

            auto error = ErrorCode::FromHResult(hr);

            if (!error.IsSuccess())
            {
                errorMessage = wformatString("Acknowledge() end of replication stream failed: hr={0}", hr);

                return error;
            }
        }

        this->CancelInternal();

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::SecondaryPump::GetOperationBuffers(
        ComPointer<IFabricOperation> const & operationCPtr,
        __out vector<Serialization::FabricIOBuffer> & buffers,
        __out ULONG & totalSize,
        __out wstring & errorMessage)
    {
        buffers.clear();
        totalSize = 0;

        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = NULL;
        HRESULT hr = operationCPtr->GetData(&bufferCount, &replicaBuffers);

        if (!SUCCEEDED(hr) || bufferCount < 1)
        {
            errorMessage = wformatString("GetData() failed: bufferCount={0} HR={1}", bufferCount, hr);
                        
            Assert::TestAssert("{0}", errorMessage);

            auto error = ErrorCode::FromHResult(hr);

            if (error.IsSuccess())
            {
                error = ErrorCodeValue::StoreUnexpectedError;
            }

            return error;
        }
                
        for (ULONG i = 0; i < bufferCount; i++)
        {
            totalSize += replicaBuffers[i].BufferSize;
            buffers.push_back(Serialization::FabricIOBuffer(replicaBuffers[i].Buffer, replicaBuffers[i].BufferSize));
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessCopyOperation(
        AsyncOperationSPtr const & pumpOperation,
        ComPointer<IFabricOperation> const & operationCPtr,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        __out vector<ReplicationOperation> & replicationOperations,
        __out ::FABRIC_SEQUENCE_NUMBER & lastQuorumAcked,
        __out wstring & errorMessage)
    {
        replicationOperations.clear();
        lastQuorumAcked = 0;
        errorMessage.clear();

        vector<Serialization::FabricIOBuffer> buffers;
        ULONG totalSize;

        auto error = this->GetOperationBuffers(operationCPtr, buffers, totalSize, errorMessage);

        if (!error.IsSuccess())
        {
            return error;
        }

        CopyOperation copyOperation;
        error = FabricSerializer::Deserialize(copyOperation, totalSize, buffers);

        if (!error.IsSuccess())
        {
            errorMessage = L"failed to deserialize copy operation";

            return error;
        }

        error = copyOperation.TakeReplicationOperations(replicationOperations);

        if (!error.IsSuccess())
        {
            errorMessage = L"failed to take copy operations";

            return error;
        }

        ReplicatedStoreEventSource::Trace->SecondaryPumpedCopy(
            this->PartitionedReplicaId,
            operationLsn,
            replicationOperations.size(),
            totalSize,
            pumpOperation->CompletedSynchronously,
            copyOperation.CopyType);

        switch (copyOperation.CopyType)
        {
        case CopyType::FileStreamFullCopy:
        case CopyType::FileStreamRebuildCopy:
            isFullCopy_ = true;

            error = this->ApplyFileStreamFullCopyOperation(copyOperation.CopyType, operationLsn, copyOperation.FileStreamData, errorMessage);

            if (error.IsSuccess())
            {
                // We are building the physical data files from the stream, so there will
                // be no logical database operations to apply. Acknowledge here.
                //
                HRESULT hr = operationCPtr->Acknowledge();

                error = ErrorCode::FromHResult(hr);

                if (!error.IsSuccess())
                {
                    errorMessage = wformatString(
                        "Acknowledge() file stream full copy failed [{0}]: LSN={1} hr={2}",
                        copyOperation.CopyType,
                        operationLsn,
                        hr);
                }
            }

            return error;

        case CopyType::FirstFullCopy:
            isFullCopy_ = true;

            error = replicatedStore_.GetLocalStoreForFullCopy(copyLocalStore_); // inout

            if (error.IsSuccess())
            {
                this->SetLogicalCopyStreamComplete(false);
            }
            else
            {
                errorMessage = L"GetLocalStoreForFullCopy() failed";
            }

            return error;

        case CopyType::FirstSnapshotPartialCopy:
            isFullCopy_ = false;

            // This happens during a partial copy if the primary cannot find a 
            // transactionally consistent point to split the copy stream into pages.
            // The remainder of the copy will occur on the snapshot database.
            //
            error = replicatedStore_.GetLocalStoreForPartialCopy(copyLocalStore_); // inout

            if (error.IsSuccess())
            {
                this->SetLogicalCopyStreamComplete(false);
            }
            else
            {
                errorMessage = L"GetLocalStoreForPartialCopy() failed";
            }

            return error;

        case CopyType::FirstPartialCopy:
            isFullCopy_ = false;

            // Primary is starting a new copy that can apply to the current database. If there
            // were any previous (incomplete) copy databases, then remove them.
            //
            // This should not happen in practice, since interrupted builds are restarted
            // on new secondary instances.
            //
            if (copyLocalStore_)
            {
                TRACE_WARNING_AND_TESTASSERT(
                    TraceComponent,
                    "{0}: restarted partial copy",
                    this->TraceId);

                error = this->TryCleanupCopyLocalStore(pumpOperation);

                if (!error.IsSuccess())
                {
                    errorMessage = L"TryCleanupCopyLocalStore() failed";

                    return error;
                }
            }

            return error;

        case CopyType::PagedCopy:
            if (!replicationOperations.empty())
            {
                auto copyLsn = replicationOperations.front().OperationLSN;

                // The copy stream should never contain LSNs out of order or split an
                // LSN across pages. If this happens, then assume that a new copy
                // stream has started and furthermore, the primary is running an old 
                // code version that does not send CopyType. Handle this condition like 
                // FirstPartialCopy.
                //
                // This should not happen in practice, since interrupted builds are restarted
                // on new secondary instances.
                //
                if (copyLsn <= lastCopyLsn_ && copyLocalStore_)
                {
                    TRACE_WARNING_AND_TESTASSERT(
                        TraceComponent,
                        "{0}: incoming copy LSN ({1}) <= last processed copy LSN ({2}): reset copy database snapshot",
                        this->TraceId,
                        copyLsn,
                        lastCopyLsn_);

                    error = this->TryCleanupCopyLocalStore(pumpOperation);
                    
                    if (!error.IsSuccess())
                    {
                        errorMessage = L"TryCleanupCopyLocalStore() failed";
                    }
                }
            }

            return error;

        default:
            errorMessage = wformatString("invalid copy type {0}", copyOperation.CopyType);

            return ErrorCodeValue::InvalidOperation;

        } // switch copy type
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessReplicationOperation(
        AsyncOperationSPtr const & pumpOperation,
        ComPointer<IFabricOperation> const & operationCPtr,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        __out vector<ReplicationOperation> & replicationOperations,
        __out ::FABRIC_SEQUENCE_NUMBER & lastQuorumAcked,
        __out wstring & errorMessage)
    {
        replicationOperations.clear();
        lastQuorumAcked = 0;
        errorMessage.clear();

        vector<Serialization::FabricIOBuffer> buffers;
        ULONG totalSize;

        auto error = this->GetOperationBuffers(operationCPtr, buffers, totalSize, errorMessage);

        if (!error.IsSuccess())
        {
            return error;
        }

        AtomicOperation atomicOperation;
        error = FabricSerializer::Deserialize(atomicOperation, totalSize, buffers);

        if (!error.IsSuccess())
        {
            errorMessage = L"failed to deserialize replication operation";

            return error;
        }

        error = atomicOperation.TakeReplicationOperations(replicationOperations);

        if (!error.IsSuccess())
        {
            errorMessage = L"failed to take replication operations";

            return error;
        }

        lastQuorumAcked = atomicOperation.LastQuorumAcked;

        ReplicatedStoreEventSource::Trace->SecondaryPumpedReplication(
            this->PartitionedReplicaId,
            operationLsn,
            lastQuorumAcked,
            replicationOperations.size(),
            totalSize,
            pumpOperation->CompletedSynchronously,
            atomicOperation.ActivityId);

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::SecondaryPump::TryCleanupCopyLocalStore(AsyncOperationSPtr const &)
    {
        auto error = copyLocalStore_->Cleanup();

        if (error.IsSuccess()) 
        { 
            copyLocalStore_.reset();

            this->SetLogicalCopyStreamComplete(false);
        }

        return error;
    }

    void ReplicatedStore::SecondaryPump::RestartDrainOperationsIfNeeded(AsyncOperationSPtr const & pumpOperation)
    {
        if (!pumpOperation->CompletedSynchronously)
        {
            this->ScheduleDrainOperations();
        }
    }

    ErrorCode ReplicatedStore::SecondaryPump::ApplyFileStreamFullCopyOperation(
        CopyType::Enum copyType,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        unique_ptr<FileStreamCopyOperationData> const & copyOperationData,
        __out wstring & errorMessage)
    {
        auto backupDir = Path::Combine(
            replicatedStore_.GetLocalStoreRootDirectory(),
            wformatString("S_{0}", replicatedStore_.ReplicaId));

        auto archiveFileName = wformatString("{0}.zip", backupDir);

        if (copyOperationData->IsFirstChunk)
        {
            this->SetLogicalCopyStreamComplete(false);

            if (fullCopyFileUPtr_)
            {
                auto error = fullCopyFileUPtr_->Close2();

                if (!error.IsSuccess())
                {
                   errorMessage = wformatString("failed to close {0}", fullCopyFileUPtr_->FileName);

                   return error;
                }

                error = fullCopyFileUPtr_->Delete();

                if (!error.IsSuccess())
                {
                    errorMessage = wformatString("failed to delete {0}", fullCopyFileUPtr_->FileName);

                    return error;
                }
            }

            fullCopyFileUPtr_ = make_unique<File>();

            if (Directory::Exists(backupDir))
            {
                auto error = Directory::Delete(backupDir, true);

                if (!error.IsSuccess())
                {
                    errorMessage = wformatString("failed to delete {0}", backupDir);

                    return error;
                }
            }

            if (File::Exists(archiveFileName))
            {
                auto error = File::Delete2(archiveFileName);

                if (!error.IsSuccess())
                {
                    errorMessage = wformatString("failed to delete {0}", archiveFileName);

                    return error;
                }
            }

            auto error = fullCopyFileUPtr_->TryOpen(
                archiveFileName,
                FileMode::CreateNew,
                FileAccess::Write,
                FileShare::None,
                FileAttributes::Normal);

            if (!error.IsSuccess())
            { 
                fullCopyFileUPtr_.reset();

                errorMessage = wformatString("failed to open {0}", archiveFileName);

                return error;
            }

        } // IsFirstChunk

        vector<byte> buffer;
        auto error = copyOperationData->TakeData(buffer);

        if (!error.IsSuccess())
        {
            errorMessage = wformatString("failed to get data for {0}", archiveFileName);

           return error;
        }

        DWORD bytesWritten = 0;

        error = fullCopyFileUPtr_->TryWrite2(buffer.data(), static_cast<int>(buffer.size()), bytesWritten); 

        if (!error.IsSuccess())
        {
            errorMessage = wformatString("failed to write {0}", archiveFileName);

           return error;
        }

       WriteNoise(
           TraceComponent, 
           "{0} wrote file stream data {1}: LSN={2} bytes={3}",
           this->TraceId, 
           fullCopyFileUPtr_->FileName,
           operationLsn,
           bytesWritten);

        if (copyOperationData->IsLastChunk)
        {
            int64 fileSize = 0;
            error = fullCopyFileUPtr_->GetSize(fileSize);

            if (!error.IsSuccess())
            {
                errorMessage = wformatString("failed to get file size {0}", fullCopyFileUPtr_->FileName);

                return error;
            }

            error = fullCopyFileUPtr_->Close2();

            if (!error.IsSuccess())
            {
                errorMessage = wformatString("failed to close {0}", fullCopyFileUPtr_->FileName);

                return error;
            }

            replicatedStore_.SetQueryStatusDetails(wformatString(
                GET_STORE_RC(FS_FullCopy_Unzip), 
                DateTime::Now(),
                fileSize));

            Stopwatch stopwatch;

            stopwatch.Start();

            error = Directory::ExtractArchive(fullCopyFileUPtr_->FileName, backupDir);

            auto elapsedExtract = stopwatch.Elapsed;

            if (!error.IsSuccess())
            {
               errorMessage = wformatString("failed to extract {0} -> {1}", fullCopyFileUPtr_->FileName, backupDir);

               return error;
            }

            error = fullCopyFileUPtr_->Delete();

            if (!error.IsSuccess())
            {
               errorMessage = wformatString("failed to delete {0}", fullCopyFileUPtr_->FileName);

               return error;
            }
            
            auto elapsedDelete = stopwatch.Elapsed;

            error = replicatedStore_.FinishFileStreamFullCopy(copyType, backupDir);

            if (!error.IsSuccess())
            {
               errorMessage = wformatString("failed to finish file stream copy {0}", backupDir);

               return error;
            }

            auto elapsedRestore = stopwatch.Elapsed;

            fullCopyFileUPtr_.reset();

            shouldNotifyCopyComplete_ = true;

            WriteInfo(
                TraceComponent, 
                "{0} applied file stream full copy - time: extract={1} delete={2} restore={3}",
                this->TraceId, 
                elapsedExtract,
                elapsedDelete,
                elapsedRestore);

        } // IsLastChunk

        return ErrorCodeValue::Success;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ApplyLocalOperations(
        CopyOperation && copyOperation,
        ::FABRIC_SEQUENCE_NUMBER mockLSN)
    {
        auto replicationOperations = make_shared<vector<ReplicationOperation>>();
        auto error = copyOperation.TakeReplicationOperations(*replicationOperations);

        if (!error.IsSuccess()) { return error; }

        auto operationType = replicationOperations->front().Operation;
        if (operationType != ReplicationOperationType::Copy)
        {
            TRACE_ERROR_AND_TESTASSERT(TraceComponent, "{0} unsupported local apply type: expected=Copy actual={1}", this->TraceId, operationType);

            return ErrorCodeValue::InvalidOperation;
        }

        wstring errorMessage;
        error = this->ApplyOperationsWithRetry(
            ComPointer<IFabricOperation>(),
            replicationOperations,
            mockLSN,
            0,
            errorMessage);

        if (!error.IsSuccess())
        {
            WriteInfo(TraceComponent, "{0}: {1}: error={2}", this->TraceId, errorMessage, error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ApplyOperationsWithRetry(
        ComPointer<IFabricOperation> const & operationCPtr,
        shared_ptr<vector<ReplicationOperation>> const & replicationOperations,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        ::FABRIC_SEQUENCE_NUMBER lastQuorumAcked,
        __out wstring & errorMessage)
    {
        if (replicationOperations->empty())
        {
            errorMessage = L"no replication operations to apply";

            return ErrorCodeValue::InvalidOperation;
        }

        // Process notification before applying on secondary (notification 
        // may or may not block depending on mode requested by application)
        //
        switch (replicationOperations->front().Operation)
        {
        case ReplicationOperationType::Insert:
        case ReplicationOperationType::Update:
        case ReplicationOperationType::Delete:
        {
            auto error = replicatedStore_.Notifications->NotifyReplication(
                *replicationOperations,
                operationLsn,
                lastQuorumAcked);

            if (!error.IsSuccess())
            {
                errorMessage = L"NotifyReplication() failed";

                return error;
            }
            break;
        }}

        int retryCount = StoreConfig::GetConfig().SecondaryApplyRetryCount;
        int retryRemaining = retryCount;

        ErrorCode error(ErrorCodeValue::Success);

        do
        {
            // Retries must always occur in a new transaction to allow
            // recovery from write conflicts and must replay all operations.
            //
            // (Write conflicts can occur between tombstone updates and tombstone
            // cleanup if the same key is deleted/re-inserted).
            //
            TransactionSPtr txSPtr;
            error = this->GetCurrentLocalStore()->CreateTransaction(txSPtr);
            if (!error.IsSuccess())
            {
                errorMessage = L"CreateTransaction() failed";

                return error;
            }

            size_t updatedTombstoneCount = 0;
            error = this->ApplyOperations(
                txSPtr, 
                *replicationOperations, 
                operationLsn, 
                lastQuorumAcked, 
                updatedTombstoneCount, // out
                errorMessage); // out

            if (error.IsSuccess())
            {
                if (this->ApproveNewOperationCreation())
                {
                    auto commitOperation = std::make_shared<CommitAndAcknowledgeAsyncOperation>(
                        *this,
                        move(txSPtr),
                        operationCPtr,
                        operationLsn,
                        replicationOperations,
                        updatedTombstoneCount,
                        [this](AsyncOperationSPtr const & operation){ this->OnCommitComplete(operation, false); },
                        this->CreateAsyncOperationRoot());

                    lastLsnProcessed_ = operationLsn;

                    commitOperation->Start(commitOperation);

                    this->OnCommitComplete(commitOperation, true);
#if defined(PLATFORM_UNIX) && defined(SYNC_COMMIT)
                    this->commitCompleteEvent_.WaitOne(TimeSpan::MaxValue);
                    this->commitCompleteEvent_.Reset();
#endif
                }
                else
                {
                    errorMessage = L"pump closed while committing";
                    error = ErrorCodeValue::ObjectClosed;
                }

                return error;
            }
            else
            {
                txSPtr->Rollback();

                if (IsRetryable(error))
                {
                    if (retryRemaining > 0)
                    {
                        WriteInfo(
                            TraceComponent,
                            "{0} retrying apply: remaining={1} LSN={2}",
                            this->TraceId,
                            retryRemaining,
                            operationLsn);

                        Sleep(StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds);
                    }
                    else
                    {
                        // no-op: exit retry loop and fault replica
                    }
                }
                else
                {
                    // should already be set by ApplyOperations()
                    //
                    if (errorMessage.empty())
                    {
                        errorMessage = wformatString(L"error details missing: LSN={0}", operationLsn);

                        Assert::TestAssert("{0}", errorMessage);
                    }

                    return error;
                }
            }

        } while (--retryRemaining >= 0);

        errorMessage = wformatString("retries exhausted: LSN={0}", operationLsn);

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ApplyOperations(
        TransactionSPtr const & txSPtr,
        vector<ReplicationOperation> const & replicationOperations,
        ::FABRIC_SEQUENCE_NUMBER operationLsn,
        ::FABRIC_SEQUENCE_NUMBER,
        __out size_t & updatedTombstoneCount,
        __out wstring & errorMessage)
    {
        updatedTombstoneCount = 0;
        size_t tombstoneIndex = 0;

        ErrorCode error(ErrorCodeValue::Success);

        for (auto const & operation : replicationOperations)
        {
            ReplicatedStoreEventSource::Trace->SecondaryApply(
                this->PartitionedReplicaId,
                operation.Operation,
                operation.Type,
                operation.Key,
                operation.OperationLSN,
                operationLsn);

            switch (operation.Operation)
            {
            case ReplicationOperationType::Copy:
                error = this->ApplyCopyOperation(txSPtr, operation);

                break;

            case ReplicationOperationType::Insert:
                error = InsertOrUpdate(
                    txSPtr,
                    operation.Type,
                    operation.Key,
                    operation.Key,
                    operation.Bytes.data(),
                    operation.Bytes.size(),
                    operationLsn,
                    operation.GetLastModifiedOnPrimaryUtcAsUPtr());

                break;

            case ReplicationOperationType::Update:
                //
                // Since writes are not blocked on the primary during copy,
                // it is possible for an update to appear in the replication
                // stream for data items that never appeared in the copy stream.
                //
                error = UpdateOrInsert(
                    txSPtr,
                    operation.Type,
                    operation.Key,
                    operation.NewKey,
                    operation.Bytes.data(),
                    operation.Bytes.size(),
                    operationLsn,
                    operation.GetLastModifiedOnPrimaryUtcAsUPtr());

                break;

            case ReplicationOperationType::Delete:
                error = this->DeleteIfDataItemExists(
                    txSPtr, 
                    operation.Type, 
                    operation.Key, 
                    operationLsn);

                if (error.IsSuccess())
                {
                    bool shouldCountTombstone = false;
                    error = replicatedStore_.FinalizeTombstone(
                        txSPtr,
                        operation.Type,
                        operation.Key,
                        operationLsn,
                        tombstoneIndex++,
                        shouldCountTombstone);

                    if (error.IsSuccess() && shouldCountTombstone)
                    {
                        ++updatedTombstoneCount;
                    }
                }

                break;

            default:
                errorMessage = wformatString("invalid replication operation type {0}", operation.Operation); 

                return ErrorCodeValue::InvalidOperation; 

            } // switch operation type

            if (error.IsSuccess() && replicatedStore_.test_SecondaryApplyFaultInjectionEnabled_)
            {
                ErrorCodeValue::Enum injectedError;
                if (FaultInjector::GetGlobalFaultInjector().TryReadError(FaultInjector::ReplicatedStoreApplyTag, injectedError))
                {
                    error = ErrorCode(injectedError);
                }
            }

            // Avoid building string in typical success case
            if (error.IsError(ErrorCodeValue::ObjectClosed))
            {
                errorMessage = wformatString("store closed while applying {0}", operation.Operation); 

                return error;
            }

            if (!error.IsSuccess())
            {
                errorMessage = wformatString(
                    "{0} failed to apply: [{1}] type='{2}' key='{3}' LSN={4} bytes={5} error={6}",
                    this->TraceId,
                    operation.Operation,
                    operation.Type,
                    operation.Key,
                    operationLsn,
                    operation.Bytes.size(),
                    error);

                return error;
            }

        } // for each replication operation

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ApplyCopyOperation(TransactionSPtr const & txSPtr, ReplicationOperation const & operation)
    {
        ErrorCode error(ErrorCodeValue::Success);

        if (operation.Type == Constants::TombstoneDataType)
        {
            error = this->ProcessTombstone(txSPtr, operation);
        }
        else if (operation.Type == Constants::ProgressDataType)
        {
            if (operation.Key == Constants::TombstoneLowWatermarkDataName)
            {
                bool shouldAccept = false;
                error = this->ProcessTombstoneLowWatermark(txSPtr, operation, shouldAccept);

                if (error.IsSuccess() && !shouldAccept)
                {
                    lastCopyLsn_ = operation.OperationLSN;

                    return ErrorCodeValue::Success;
                }
            }

            // We may need to update the current epoch several times during the course
            // of the copy and want to keep the same sequence number so that the highest
            // sequence number on the secondary never exceeds the highest sequence number
            // on the primary (which would look like false progress)
            //
            error = this->DeleteIfDataItemExists(txSPtr, operation.Type, operation.Key, ILocalStore::SequenceNumberIgnore);

            if (error.IsSuccess())
            {
                if (operation.Key == Constants::EpochHistoryDataName)
                {
                    error = this->ProcessEpochHistory(txSPtr, operation);
                }
                else if (operation.Key == Constants::CurrentEpochDataName)
                {
                    error = this->ProcessEpochUpdate(txSPtr, operation);
                }
            }
        } // progress data

        if (error.IsSuccess())
        {
            error = InsertOrUpdate(
                txSPtr,
                operation.Type,
                operation.Key,
                operation.Key,
                operation.Bytes.data(),
                operation.Bytes.size(),
                // Keep the sequence number on the data item
                // rather than the copy operation
                operation.OperationLSN,
                operation.GetLastModifiedOnPrimaryUtcAsUPtr());

            if (error.IsSuccess())
            {
                lastCopyLsn_ = operation.OperationLSN;
            }
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessTombstone(
        TransactionSPtr const & txSPtr, 
        ReplicationOperation const & operation)
    {
        wstring dataType;
        wstring dataKey;

        if (!replicatedStore_.TryParseTombstoneKey1(operation.Key, dataType, dataKey))
        {
            TombstoneData tombstone;
            auto error = FabricSerializer::Deserialize(tombstone, const_cast<vector<byte>&>(operation.Bytes));

            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    TraceComponent,
                    "{0} failed to deserialize tombstone: type={1} key={2} error={3}",
                    this->TraceId,
                    operation.Type,
                    operation.Key,
                    error);

                return error; 
            }

            dataType = tombstone.LiveEntryType;
            dataKey = tombstone.LiveEntryKey;
        }

        return this->DeleteIfDataItemExists(txSPtr, dataType, dataKey, operation.OperationLSN);
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessTombstoneLowWatermark(
        TransactionSPtr const & txSPtr, 
        ReplicationOperation const & operation,
        __out bool & shouldAccept)
    {
        TombstoneLowWatermarkData incomingLowWatermark;
        auto error = FabricSerializer::Deserialize(incomingLowWatermark, const_cast<vector<byte>&>(operation.Bytes));

        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "{0} failed to deserialize low watermark data: error={1}",
                this->TraceId,
                error);

            return error;
        }

        // Secondary performs tombstone cleanup independent from primary, so low watermark data
        // from the primary is only applicable during full copy
        //
        if (isFullCopy_)
        {
            WriteInfo(
                TraceComponent,
                "{0} applying tombstone low watermark: incoming={1}",
                this->TraceId,
                incomingLowWatermark);

            shouldAccept = true;
        }
        else
        {
            // Read existing low watermark for debugging purposes only
            //
            TombstoneLowWatermarkData existingLowWatermark;
            error = replicatedStore_.GetTombstoneLowWatermark(txSPtr, existingLowWatermark);

            if (!error.IsSuccess())
            {
                WriteInfo(
                    TraceComponent,
                    "{0} failed to read existing low watermark: error={1}",
                    this->TraceId,
                    error);

                return error;
            }

            WriteInfo(
                TraceComponent,
                "{0} skipping tombstone low watermark update: incoming={1} existing={2}",
                this->TraceId,
                incomingLowWatermark,
                existingLowWatermark);

            shouldAccept = false;
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessEpochHistory(TransactionSPtr const &, ReplicationOperation const & operation)
    {
        ProgressVectorData progressVector;
        auto error = FabricSerializer::Deserialize(progressVector, const_cast<vector<byte>&>(operation.Bytes));

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} received epoch history: {1}",
                this->TraceId,
                progressVector);

            // Epoch history update is always the last copy operation
            // in the copy stream
            //
            this->SetLogicalCopyStreamComplete(true);
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::ProcessEpochUpdate(TransactionSPtr const &, ReplicationOperation const & operation)
    {
        CurrentEpochData currentEpoch;
        auto error = FabricSerializer::Deserialize(currentEpoch, const_cast<vector<byte>&>(operation.Bytes));

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0} received current epoch: {1}",
                this->TraceId,
                currentEpoch);
        }

        return error;
    }

    bool ReplicatedStore::SecondaryPump::IsRetryable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
            case ErrorCodeValue::ObjectClosed:
            case ErrorCodeValue::InvalidOperation:
                return false;

            default:
                return true;
        }
    }

    void ReplicatedStore::SecondaryPump::OnCommitComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

#if defined(PLATFORM_UNIX) && defined(SYNC_COMMIT)
        commitCompleteEvent_.Set();
#endif

        auto error = operation->Error;
        
        if (!error.IsSuccess())
        {
            auto casted = AsyncOperation::Get<CommitAndAcknowledgeAsyncOperation>(operation);

            this->TransientFaultReplica(
                error,
                wformatString("commit transaction failed: LSN={0}", casted->OperationLSN),
                false); // scheduleDrain
        }
    }

    ErrorCode ReplicatedStore::SecondaryPump::InsertOrUpdate(
        TransactionSPtr const & txSPtr,
        std::wstring const & type, 
        std::wstring const & key, 
        std::wstring const & newKey, 
        __in void const * value,
        size_t valueSizeInBytes,
        _int64 operationLsn,
        std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr)
    {
        auto const & localStore = this->GetCurrentLocalStore();

#if defined(PLATFORM_UNIX)
        auto err = localStore->Lock(txSPtr, type, key);
        if (!err.IsSuccess() && err.ReadValue() != ErrorCodeValue::StoreRecordNotFound)
        {
            return err;
        }
#endif
        auto error = localStore->Insert(
            txSPtr,
            type,
            key,
            value,
            valueSizeInBytes,
            operationLsn,
            lastModifiedOnPrimaryUtcUPtr.get());

        if (error.IsSuccess())
        {
            this->AddPendingInsert(type, key, operationLsn);
        }
        else if (error.IsError(ErrorCodeValue::StoreRecordAlreadyExists))
        {
            _int64 currentOperationLsn = ILocalStore::SequenceNumberIgnore;
            error = localStore->GetOperationLSN(txSPtr, type, key, currentOperationLsn);

            if (error.IsSuccess())
            {
                error = this->DoUpdate(
                    localStore,
                    txSPtr,
                    type,
                    key,
                    newKey,
                    value,
                    valueSizeInBytes,
                    operationLsn,
                    currentOperationLsn,
                    move(lastModifiedOnPrimaryUtcUPtr));
            }
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} secondary failed InsertOrUpdate(): type = {1} key = {2} replication sequence = {3} error = {4}", 
                this->TraceId,
                type,
                key,
                operationLsn,
                error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::UpdateOrInsert(
        TransactionSPtr const & txSPtr,
        std::wstring const & type, 
        std::wstring const & key, 
        std::wstring const & newKey, 
        __in void const * value,
        size_t valueSizeInBytes,
        _int64 operationLsn,
        std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr)
    {
        auto const & localStore = this->GetCurrentLocalStore();

#if defined(PLATFORM_UNIX)
        auto err = localStore->Lock(txSPtr, type, key);
        if (!err.IsSuccess() && err.ReadValue() != ErrorCodeValue::StoreRecordNotFound)
        {
            return err;
        }
#endif

        _int64 currentOperationLsn = ILocalStore::SequenceNumberIgnore;
        auto error = localStore->GetOperationLSN(txSPtr, type, key, currentOperationLsn);
        
        if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            error = localStore->Insert(
                txSPtr,
                type,
                key,
                value,
                valueSizeInBytes,
                operationLsn,
                lastModifiedOnPrimaryUtcUPtr.get());

            if (error.IsSuccess())
            {
                this->AddPendingInsert(type, key, operationLsn);
            }
        }
        else if (error.IsSuccess())
        {
            error = this->DoUpdate(
                localStore,
                txSPtr,
                type,
                key,
                newKey,
                value,
                valueSizeInBytes,
                operationLsn,
                currentOperationLsn,
                move(lastModifiedOnPrimaryUtcUPtr));
        }

        if (!error.IsSuccess())
        {
            WriteInfo(
                TraceComponent, 
                "{0} secondary failed UpdateOrInsert(): type = {1} key = {2} replication sequence = {3} error = {4}", 
                this->TraceId,
                type,
                key,
                operationLsn,
                error);
        }

        return error;
    }

    ErrorCode ReplicatedStore::SecondaryPump::DoUpdate(
        ILocalStoreUPtr const & localStore,
        TransactionSPtr const & txSPtr,
        std::wstring const & type, 
        std::wstring const & key, 
        std::wstring const & newKey, 
        __in void const * value,
        size_t valueSizeInBytes,
        _int64 operationLsn,
        _int64 currentOperationLsn,
        std::unique_ptr<FILETIME> && lastModifiedOnPrimaryUtcUPtr)
    {
        // Support multiple updates to the same key within the same transaction
        //
        if (operationLsn >= currentOperationLsn)
        {
            return localStore->Update(
                txSPtr,
                type,
                key,
                0,
                newKey,
                value,
                valueSizeInBytes,
                operationLsn,
                lastModifiedOnPrimaryUtcUPtr.get());
        }
        else
        {
            // This can happen during file stream full build when an entry is updated
            // multiple times just as the full build backup occurs on the primary. The full build
            // contents will contain only the latest version of the entry (LSN = K) while
            // the replication stream that's buffered during the full build will contain all
            // updates to this entry in LSN order (LSNs <= K).
            //
            WriteInfo(
                TraceComponent, 
                "{0} secondary ignored DoUpdate(): type='{1}' key='{2}' lsn=(incoming={3} existing={4})", 
                this->TraceId,
                type,
                key,
                operationLsn,
                currentOperationLsn);

            return ErrorCodeValue::Success;
        }
    }

    ErrorCode ReplicatedStore::SecondaryPump::DeleteIfDataItemExists(
        TransactionSPtr const & txSPtr,
        wstring const & type,
        wstring const & key,
        FABRIC_SEQUENCE_NUMBER operationLsn)
    {
        auto const & localStore = this->GetCurrentLocalStore();

#if defined(PLATFORM_UNIX)
        auto err = localStore->Lock(txSPtr, type, key);
        if (!err.IsSuccess() && err.ReadValue() != ErrorCodeValue::StoreRecordNotFound)
        {
            return err;
        }
#endif

        _int64 currentOperationLsn = ILocalStore::SequenceNumberIgnore;
        auto error = localStore->GetOperationLSN(txSPtr, type, key, currentOperationLsn);

        // Delete operation LSN can potentially be less than the live entry LSN
        // when processing a tombstone during copy and there's a live entry
        // that was re-inserted after the delete. This can also happen
        // if the queued replication stream delete is behind the progress included
        // by a full build (i.e. the full build includes the delete, re-insert).
        //
        if (error.IsSuccess() && operationLsn != ILocalStore::SequenceNumberIgnore && operationLsn < currentOperationLsn)
        {
            WriteInfo(
                TraceComponent, 
                "{0} no-op delete for higher LSN live entry: type='{1}' key='{2}' lsn=(delete={3} existing={4})",
                this->TraceId,
                type,
                key,
                operationLsn,
                currentOperationLsn);

            return ErrorCodeValue::Success;
        }
        else if (error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            FABRIC_SEQUENCE_NUMBER pendingLsn = 0;
            if (this->ContainsPendingInsert(type, key, operationLsn, pendingLsn))
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} block applying delete on pending insert: type='{1}' key='{2}' lsn={3} pending={4}",
                    this->TraceId,
                    type,
                    key,
                    operationLsn,
                    pendingLsn);

                // Block this delete if there is a pending uncommitted insert that does not
                // show up in the delete tx view. Otherwise, we'll end up dropping the delete 
                // on this secondary (see EseLocalStore implementation of Delete).
                //
                // Returning an error here will cause retrying of the apply in a new tx.
                //
                return ErrorCodeValue::StoreWriteConflict;
            }
            else
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} no-op delete for non-existent entry: type='{1}' key='{2}' lsn={3}",
                    this->TraceId,
                    type,
                    key,
                    operationLsn);

                return ErrorCodeValue::Success;
            }
        }

        return localStore->Delete(
            txSPtr,
            type,
            key,
            currentOperationLsn);
    }

    void ReplicatedStore::SecondaryPump::AddPendingInsert(wstring const & type, wstring const & key, FABRIC_SEQUENCE_NUMBER lsn)
    {
        auto entry = PendingInsertMetadata::Create(type, key, lsn);
        auto hash = entry->GetHash();

        AcquireWriteLock lock(pendingInsertsLock_);

        auto findIt = pendingInserts_.find(hash);
        if (findIt == pendingInserts_.end() || findIt->second->Lsn < lsn)
        {
            pendingInserts_[hash] = entry;
        }
    }

    void ReplicatedStore::SecondaryPump::RemovePendingInserts(vector<ReplicationOperation> const & replicationOperations, FABRIC_SEQUENCE_NUMBER lsn)
    {
        AcquireWriteLock lock(pendingInsertsLock_);

        for (auto const & repl : replicationOperations)
        {
            if (repl.Operation == ReplicationOperationType::Copy || 
                repl.Operation == ReplicationOperationType::Insert || 
                repl.Operation == ReplicationOperationType::Update)
            {
                auto hash = PendingInsertMetadata::GetHash(repl.Type, repl.Key);

                auto findIt = pendingInserts_.find(hash);
                if (findIt != pendingInserts_.end() && findIt->second->Lsn <= lsn)
                {
                    pendingInserts_.erase(hash);
                }
            }
        }
    }

    bool ReplicatedStore::SecondaryPump::ContainsPendingInsert(wstring const & type, wstring const & key, FABRIC_SEQUENCE_NUMBER lsn, __out FABRIC_SEQUENCE_NUMBER & pendingLsn)
    {
        auto hash = PendingInsertMetadata::GetHash(type, key);

        AcquireReadLock lock(pendingInsertsLock_);

        auto findIt = pendingInserts_.find(hash);

        if (findIt != pendingInserts_.end())
        {
            pendingLsn = findIt->second->Lsn;

            return (pendingLsn < lsn);
        }
        else
        {
            return false;
        }
    }
        
    bool ReplicatedStore::SecondaryPump::ApproveNewOperationCreation()
    {
        AcquireReadLock acquire(thisLock_);

        if (isCanceled_)
        {
            return false;
        }
        ++pendingOperationsCount_;
        return true;
    }

    // Do not call cancel under LocalStorelock
    void ReplicatedStore::SecondaryPump::CancelInternal()
    {
        {
            AcquireWriteLock acquire(thisLock_);
            if (isCanceled_)
            {
                return;
            }
            isCanceled_ = true;

            pumpState_ = PumpClosed;
            operationStream_.SetNoAddRef(NULL);
        }

        WriteInfo(TraceComponent, "{0} canceling secondary pump", this->TraceId);

        // We start with operation count set to 1
        this->OnPendingOperationCompletion();
    }

    void ReplicatedStore::SecondaryPump::OnPendingOperationCompletion()
    {
        if (0 == --pendingOperationsCount_)
        {
            replicatedStore_.Notifications->DrainNotifications();

            WriteInfo(TraceComponent, "{0} calling replicated store to finish secondary pump.", this->TraceId);

            // This may call CleanUp() on the ReplicatedStore, which takes the LocalStoreLock.
            // So do NOT hold LocalStoreLock while calling Cancel().
            //
            replicatedStore_.FinishSecondaryPump();
        }
    }

    // This method is for the test or parent store to call, while not in the pump code path.
    // As a result, we do not schedule a drain here.
    //
    void ReplicatedStore::SecondaryPump::Cancel()
    {
        this->TryFaultStreamOrCancel(false /*scheduleDrain*/);
    }

    void ReplicatedStore::SecondaryPump::TransientFaultReplica(
        ErrorCode const & error,
        std::wstring const & message,
        bool scheduleDrain)
    {
        this->FaultReplica(FABRIC_FAULT_TYPE_TRANSIENT, error, message, scheduleDrain);
    }

    void ReplicatedStore::SecondaryPump::FaultReplica(
        FABRIC_FAULT_TYPE faultType,
        ErrorCode const & error,
        std::wstring const & message,
        bool scheduleDrain)
    {
        // Don't need to restart replica if it's already closing (the replicator state changed due to Close/ChangeRole).
        //
        bool shouldRestartReplica = (!error.IsError(ErrorCodeValue::ObjectClosed) && !error.IsError(ErrorCodeValue::InvalidState));

        if (shouldRestartReplica)
        {
            WriteWarning(TraceComponent, "{0}: fault({1}): error={2} drain={3}", this->TraceId, message, error, scheduleDrain);
        }
        else
        {
            WriteInfo(TraceComponent, "{0}: fault({1}): error={2} drain={3}", this->TraceId, message, error, scheduleDrain);
        }

        if (!this->TryFaultStreamOrCancel(scheduleDrain, faultType))
        {
            if (shouldRestartReplica)
            {
                replicatedStore_.ReportFault(error, faultType);
            }
        }
    }

    bool ReplicatedStore::SecondaryPump::TryFaultStreamOrCancel(bool scheduleDrain, FABRIC_FAULT_TYPE faultType)
    {
        bool reportedStreamFault = false;

        if (replicatedStore_.EnableStreamFaultsAndEOSOperationAck)
        {
            isStreamFaulted_.store(true);

            ComPointer<IFabricOperationStream> streamCPtr;
            {
                AcquireReadLock lock(thisLock_);

                streamCPtr = operationStream_;
            }
            
            if (streamCPtr)
            {
                ComPointer<IFabricOperationStream2> stream2CPtr;
                HRESULT hr = streamCPtr->QueryInterface(IID_IFabricOperationStream2, reinterpret_cast<void**>(stream2CPtr.InitializationAddress()));
                ASSERT_IFNOT(SUCCEEDED(hr), "{0}: Failed to query IFabricOperationStream2 Interface", this->TraceId);
                hr = stream2CPtr->ReportFault(faultType);
                reportedStreamFault = SUCCEEDED(hr);
                WriteInfo(TraceComponent, "{0}: Report fault on the stream returned HR={1}", this->TraceId, hr);

                if (scheduleDrain)
                {
                    WriteInfo(TraceComponent, "{0}: TryFaultStreamOrCancel() scheduling drain", this->TraceId);
                    this->ScheduleDrainOperations();
                }
            }
        }

        if (!reportedStreamFault)
        {
            // If we failed to fault the stream, it is due to 1 of the following reasons:
            // 1. EOS Feature is disabled -> In which case CancelInternal will stop the pump
            // 2. Stream was invalid -> In which case, there is no operation to pump and hence CancelInternal will stop the pump
            // 3. Stream was valid but report fault failed -> This again means the stream released its reference to the replicator and we no longer need to pump
            this->CancelInternal();
        }

        return reportedStreamFault;
    }

    // ************
    // Test helpers
    // ************

    // This method is for the test or parent store to call, while not in the pump code path
    // As a result, we do not schedule a drain here
    //
    void ReplicatedStore::SecondaryPump::Test_FaultStreamAndStopSecondaryPump(FABRIC_FAULT_TYPE faultType)
    {
        this->FaultReplica(
            faultType,
            ErrorCodeValue::OperationFailed,
            L"Test_FaultStreamAndStopSecondaryPump",
            false);
    }

    bool ReplicatedStore::SecondaryPump::Test_GetCopyStreamComplete()
    {
        return isLogicalCopyStreamComplete_;
    }

#if defined(PLATFORM_UNIX)
    bool ReplicatedStore::SecondaryPump::Test_IsPumpStateClosed()
    {
        return pumpState_ == PumpClosed;
    }
#endif

    void ReplicatedStore::SecondaryPump::SetLogicalCopyStreamComplete(bool value)
    {
        isLogicalCopyStreamComplete_ = value;
        shouldNotifyCopyComplete_ = value;
    }

    // *********************
    // ComPumpAsyncOperation
    // *********************

    ReplicatedStore::SecondaryPump::ComPumpAsyncOperation::ComPumpAsyncOperation(
        ComPointer<IFabricOperationStream> const & operationStream,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : ComProxyAsyncOperation(callback, root)
        , operationStream_(operationStream)
    {
        ReplicatedStoreEventSource::Trace->SecondaryPumpCtor(TraceThis);
    }

    ReplicatedStore::SecondaryPump::ComPumpAsyncOperation::~ComPumpAsyncOperation()
    {
        ReplicatedStoreEventSource::Trace->SecondaryPumpDtor(TraceThis);
    }

    HRESULT ReplicatedStore::SecondaryPump::ComPumpAsyncOperation::BeginComAsyncOperation(
        __in IFabricAsyncOperationCallback * callback,
        __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        ASSERT_IFNOT(operationStream_, "OperationStream cannot be empty during ComPumpAsyncOperation");
        HRESULT hr = operationStream_->BeginGetOperation(
            callback,
            operationContext);

        return ComUtility::OnPublicApiReturn(hr);
    }

    HRESULT ReplicatedStore::SecondaryPump::ComPumpAsyncOperation::EndComAsyncOperation(
        __in IFabricAsyncOperationContext * operationContext)
    {
        if (operationContext == NULL) { return ComUtility::OnPublicApiReturn(E_POINTER); }

        HRESULT hr = operationStream_->EndGetOperation(
            operationContext,
            pumpedOperation_.InitializationAddress());

        return ComUtility::OnPublicApiReturn(hr);
    }

    ErrorCode ReplicatedStore::SecondaryPump::ComPumpAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation,
        __out ComPointer<IFabricOperation> & pumpedOperation)
    {
        auto casted = AsyncOperation::End<ComPumpAsyncOperation>(asyncOperation);

        if (casted->Error.IsSuccess())
        {
            pumpedOperation.Swap(casted->pumpedOperation_);
        }

        return casted->Error;
    }

    // **********************************
    // CommitAndAcknowledgeAsyncOperation
    // **********************************

    ReplicatedStore::SecondaryPump::CommitAndAcknowledgeAsyncOperation::CommitAndAcknowledgeAsyncOperation(
        ReplicatedStore::SecondaryPump & owner,
        ILocalStore::TransactionSPtr && transactionSPtr,
        ComPointer<IFabricOperation> const &operationCPtr,
        FABRIC_SEQUENCE_NUMBER operationLsn,
        shared_ptr<vector<ReplicationOperation>> const & replicationOperations,
        size_t updatedTombstoneCount,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root, true),
        PartitionedReplicaTraceComponent(owner),
        owner_(owner),
        transactionSPtr_(std::move(transactionSPtr)),
        operationCPtr_(operationCPtr),
        operationLSN_(operationLsn),
        replicationOperations_(replicationOperations),
        updatedTombstoneCount_(updatedTombstoneCount),
        localCommitStopwatch_()
    {
    }

    ReplicatedStore::SecondaryPump::CommitAndAcknowledgeAsyncOperation::~CommitAndAcknowledgeAsyncOperation()
    {
    }

    void ReplicatedStore::SecondaryPump::CommitAndAcknowledgeAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        localCommitStopwatch_.Start();

        auto operation = transactionSPtr_->BeginCommit(
            TimeSpan::MaxValue, // Use MaxAsyncCommitDelay from settings
            [this](AsyncOperationSPtr const & operation){ this->OnCommitComplete(operation, false); },
            thisSPtr);
        this->OnCommitComplete(operation, true);
    }

    void ReplicatedStore::SecondaryPump::CommitAndAcknowledgeAsyncOperation::OnCommitComplete(
        AsyncOperationSPtr const& operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        // Keep a reference since members are accessed after TryComplete()
        //
        AsyncOperationSPtr thisSPtr = operation->Parent;

        localCommitStopwatch_.Stop();

        auto error = transactionSPtr_->EndCommit(operation);

        transactionSPtr_.reset();

        this->TryComplete(thisSPtr, error);

        owner_.RemovePendingInserts(*replicationOperations_, operationLSN_);

        if (updatedTombstoneCount_ > 0)
        {
            // Schedule after commit to keep tombstone count estimate as accurate as possible
            // (otherwise, could double-count when retrying applies).
            //
            owner_.replicatedStore_.ScheduleTombstoneCleanupIfNeeded(updatedTombstoneCount_);
        }

        int64 elapsedCommitMillis = localCommitStopwatch_.Elapsed.TotalMilliseconds();
        auto slowCommitTraceThreshold = StoreConfig::GetConfig().SlowCommitTraceThreshold;

        if (elapsedCommitMillis < static_cast<int64>(slowCommitTraceThreshold.TotalPositiveMilliseconds()) 
            && !owner_.replicatedStore_.Test_GetSlowCommitTestEnabled())
        {
            ReplicatedStoreEventSource::Trace->SecondaryCommitFast(
                this->PartitionedReplicaId,
                this->OperationLSN,
                elapsedCommitMillis);
        }
        else
        {
            ReplicatedStoreEventSource::Trace->SecondaryCommitSlow(
                this->PartitionedReplicaId,
                this->OperationLSN,
                elapsedCommitMillis);

            owner_.replicatedStore_.OnSlowCommit();
        }

        if (error.IsSuccess() && operationCPtr_)
        {
            HRESULT hr = operationCPtr_->Acknowledge();

            error = ErrorCode::FromHResult(hr);

            if (!error.IsSuccess())
            {
                ::FABRIC_OPERATION_METADATA const * operationMetadata = operationCPtr_->get_Metadata();
                ::FABRIC_OPERATION_TYPE operationType = operationMetadata->Type;

                auto message = wformatString(
                    "{0} Acknowledge() operation failed [{1}]: LSN={2} hr={3}",
                    owner_.TraceId,
                    static_cast<int>(operationType),
                    this->OperationLSN,
                    hr);
                owner_.TransientFaultReplica(error, message, false/*scheduledrain*/);
            }
        }

        owner_.OnPendingOperationCompletion();
    }
}

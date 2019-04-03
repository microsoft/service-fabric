// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    Common::GlobalWString EseLocalStore::sequenceNumberColumnName = Common::make_global<std::wstring>(L"SequenceNumber");
    Common::GlobalWString EseLocalStore::lastModifiedColumnName = Common::make_global<std::wstring>(L"LastModified");
    Common::GlobalWString EseLocalStore::lastModifiedOnPrimaryColumnName = Common::make_global<std::wstring>(L"LastModifiedOnPrimary");
    Common::GlobalWString EseLocalStore::typeColumnName = Common::make_global<std::wstring>(L"Type");
    Common::GlobalWString EseLocalStore::keyColumnName = Common::make_global<std::wstring>(L"Key");
    Common::GlobalWString EseLocalStore::valueColumnName = Common::make_global<std::wstring>(L"Value");
    Common::GlobalWString EseLocalStore::operationLSNColumnName = Common::make_global<std::wstring>(L"OperationLSN");
   
    Common::GlobalWString EseLocalStore::primaryIndexName = Common::make_global<std::wstring>(L"PrimaryIndex");
    wchar_t const EseLocalStore::primaryIndexKeysData[] = L"+Type\0+Key\0";
    Common::GlobalWString EseLocalStore::primaryIndexKeys = Common::make_global<std::wstring>(primaryIndexKeysData, sizeof(primaryIndexKeysData)/sizeof(wchar_t));

    Common::GlobalWString EseLocalStore::operationLSNIndexName = Common::make_global<std::wstring>(L"OperationLSNIndex");
    wchar_t const EseLocalStore::operationLSNIndexKeysData[] = L"+OperationLSN\0";
    Common::GlobalWString EseLocalStore::operationLSNIndexKeys = Common::make_global<std::wstring>(
        operationLSNIndexKeysData, sizeof(operationLSNIndexKeysData)/sizeof(wchar_t));

    const size_t EseLocalStore::maxTypeLengthCharacters = 256;
    const size_t EseLocalStore::maxKeyLengthCharacters = 64 * 1024;
    const size_t EseLocalStore::maxValueLengthBytes = 0x7fffffff;
    const _int64 EseLocalStore::maxOperationLSN_ = 0x7ffffffffffffff;
    const _int64 EseLocalStore::lastModifiedOnPrimaryDefVal = 0;

    Common::GlobalWString EseLocalStore::ValidationDirectorySuffix = Common::make_global<std::wstring>(L"VAL");
    Common::GlobalWString EseLocalStore::FullBackupSubDirName = Common::make_global<wstring>(L"new");
    
    static Common::atomic_long eseStoreObjectIds;

    // ------------------------------------------------------------------------
    // EseLocalStore
    //

    EseLocalStore::EseLocalStore(
        std::wstring const & directory,
        std::wstring const & fileName,
        LONG localStoreFlags)
        : instance_(),
          session_(),
          database_(),
          settings_(make_shared<EseLocalStoreSettings>()),
          directory_(directory),
          fileName_(fileName),
          sequenceNumberColumnId_(0),
          lastModifiedColumnId_(0),
          lastModifiedOnPrimaryColumnId_(0),
          typeColumnId_(0),
          keyColumnId_(0),
          valueColumnId_(0),
          operationLSNColumnId_(0),
          lock_(),
          transactionCount_(1),
          aborting_(false),
          drained_(false),
          allStoreObjectsDestructedSPtr_(std::make_shared<AutoResetEvent>(false)),
          traceId_(),
          localStoreFlags_(localStoreFlags),
          esePool_(),
          perfCounters_(),
          defragmenter_()

    {
        this->ConstructorInitialize();
    }

    EseLocalStore::EseLocalStore(
        std::wstring const & directory,
        EseLocalStoreSettings const & settings,
        LONG localStoreFlags)
        : instance_(),
          session_(),
          database_(),
          settings_(make_shared<EseLocalStoreSettings>(settings)),
          directory_(directory),
          fileName_(settings.StoreName),
          sequenceNumberColumnId_(0),
          lastModifiedColumnId_(0),
          lastModifiedOnPrimaryColumnId_(0),
          typeColumnId_(0),
          keyColumnId_(0),
          valueColumnId_(0),
          operationLSNColumnId_(0),
          lock_(),
          transactionCount_(1),
          aborting_(false),
          drained_(false),
          allStoreObjectsDestructedSPtr_(std::make_shared<AutoResetEvent>(false)),
          traceId_(),
          localStoreFlags_(localStoreFlags),
          esePool_(),
          perfCounters_(),
          defragmenter_()
    {
        this->ConstructorInitialize();
    }

    void EseLocalStore::ConstructorInitialize()
    {
        ASSERT_IF(
            directory_.empty() || fileName_.empty(), 
            "EseLocalStore.ctor invalid directory ('{0}') or file ('{1}')",
            directory_,
            fileName_);

        filePath_ = Path::Combine(directory_, fileName_);

        if (this->IsExclusiveFileLockEnabled)
        {
            exclusiveFileLock_ = Common::make_unique<ExclusiveFile>(Path::Combine(directory_, fileName_ + L".FabricLock"));
        }

        WriteInfo(Constants::StoreSource, "{0} ctor({1}, {2}): settings=[{3}]", this->TraceId, directory_, fileName_, *settings_);
    }

    EseLocalStore::~EseLocalStore()
    {
        Terminate();
        Drain();
    }

    void EseLocalStore::Drain()
    {
        {
            AcquireWriteLock acquire(lock_);
            
            if (drained_)
            {
                return;
            }
            
            drained_ = true;
        }

        auto pendingCount = --transactionCount_;

        if (pendingCount != 0)
        {
            WriteInfo(Constants::StoreSource, "{0} waiting for {1} store objects to be destructed.", this->TraceId, pendingCount);
            allStoreObjectsDestructedSPtr_->WaitOne();
            WriteInfo(Constants::StoreSource, "{0} all store objects destructed.", this->TraceId);
        }

        EseStoreEventSource::Trace->StoreDestructing(traceId_);
        ASSERT_IF(transactionCount_.load() != 0, "Transactions still exist that are using this store.");
    }

    bool EseLocalStore::StoreExists()
    {
        return File::Exists(filePath_);
    }

    ErrorCode EseLocalStore::Initialize(std::wstring const & instanceName, Federation::NodeId const & nodeId)
    {
        return this->Initialize(wformatString("{0}{1}", instanceName, nodeId));
    }

    ErrorCode EseLocalStore::Initialize(std::wstring const & instanceName)
    {
        return this->InnerInitialize(instanceName, false); // isRestoreValidation
    }

    ErrorCode EseLocalStore::InnerInitialize(
        wstring const & instanceName, 
        bool isRestoreValidation)
    {
        if (exclusiveFileLock_)
        {
            auto err =  exclusiveFileLock_->Acquire(TimeSpan::FromSeconds(1));
            if (!err.IsSuccess())
            {
                WriteInfo(
                    Constants::StoreSource,
                    "{0} failed to acquire exclusiveFileLock_: {1}",
                    this->TraceId,
                    err);

                return ErrorCodeValue::StoreInUse;
            }
        }

        traceId_ = instanceName;

        perfCounters_ = EseStorePerformanceCounters::CreateInstance(wformatString("{0}:{1}",
            instanceName,
            SequenceNumber::GetNext()));

        auto error = CreateDbDirectory();
        if (!error.IsSuccess())
        {
            return error;
        }

        instance_ = EseInstance::CreateSPtr(
            settings_, 
            perfCounters_,
            instanceName, 
            fileName_, 
            directory_,
            isRestoreValidation);
        auto instanceError = instance_->Open();
        if (!instanceError.IsSuccess()) { Terminate(); return instanceError; }

        session_ = EseSession::CreateSPtr(instance_);
        JET_ERR jetErr = session_->Initialize();

        if (JET_errSuccess == jetErr)
        {
            database_ = EseDatabase::CreateSPtr(session_);

            if (File::Exists(filePath_))
            {
                jetErr = database_->InitializeAttachAndOpen(filePath_);

                if (JET_errSuccess == jetErr)
                {
                    EseCursor cursor(database_, session_);
                    jetErr = cursor.InitializeOpenTable(Constants::TableName);
                    
                    // If the table does not exist, create it and try re-opening
                    if (JET_errObjectNotFound == jetErr)
                    {
                        jetErr = CreateInitialTablesAndColumns();

                        if (JET_errSuccess == jetErr)
                        {
                            jetErr = cursor.InitializeOpenTable(Constants::TableName);
                        }
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        jetErr = cursor.GetColumnId(sequenceNumberColumnName, sequenceNumberColumnId_);
                    }

                    if (JET_errSuccess == jetErr && this->IsOperationLSNColumnEnabled)
                    {
                        jetErr = cursor.GetColumnId(operationLSNColumnName, operationLSNColumnId_);
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        jetErr = cursor.GetColumnId(lastModifiedColumnName, lastModifiedColumnId_);
                    }

                    if (JET_errSuccess == jetErr && this->IsLastModifiedOnPrimaryColumnEnabled)
                    {
                        jetErr = cursor.GetColumnId(lastModifiedOnPrimaryColumnName, lastModifiedOnPrimaryColumnId_);

                        // If the column does not exist, create it.
                        if (JET_errColumnNotFound == jetErr)
                        {
                            WriteInfo(
                                Constants::StoreSource,
                                "{0} LastModifiedOnPrimary Column does no exist. Creating it.",
                                this->TraceId);

                            jetErr = cursor.AlterTableAddColumn(
                                lastModifiedOnPrimaryColumnName,
                                JET_coltypLongLong,
                                sizeof(_int64),
                                JET_bitColumnFixed | JET_bitColumnNotNULL,
                                const_cast<_int64 *>(&lastModifiedOnPrimaryDefVal),
                                sizeof(lastModifiedOnPrimaryDefVal),
                                lastModifiedOnPrimaryColumnId_);
                        }
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        jetErr = cursor.GetColumnId(typeColumnName, typeColumnId_);
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        jetErr = cursor.GetColumnId(keyColumnName, keyColumnId_);
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        jetErr = cursor.GetColumnId(valueColumnName, valueColumnId_);
                    }

                    if (JET_errSuccess == jetErr)
                    {
                        EseStoreEventSource::Trace->StoreInitialized(traceId_, directory_, fileName_, false);
                    }
                }
            }
            else
            {
                jetErr = database_->InitializeCreate(filePath_);

                if (JET_errSuccess == jetErr)
                {
                    jetErr = CreateInitialTablesAndColumns();
                }
            }
        }

        if (JET_errSuccess != jetErr)
        {
            Terminate();
            return this->JetToErrorCode(jetErr);
        }

        defragmenter_ = EseDefragmenter::CreateAndStart(*this);

        return ErrorCode::Success();
    }

    JET_ERR EseLocalStore::CreateInitialTablesAndColumns()
    {
        JET_ERR jetErr = JET_errSuccess;

        EseTableBuilder builder(Constants::TableName);
        _int64 initialSequenceNumber = 0;
        _int64 initialOperationLSN = 0;

        auto sequenceNumberIndex = builder.AddColumn(
            sequenceNumberColumnName,
            JET_coltypLongLong,
            sizeof(_int64),
            JET_bitColumnFixed | JET_bitColumnNotNULL,
            &initialSequenceNumber,
            sizeof(initialSequenceNumber));

        int operationLSNIndex = 0;
        if (this->IsOperationLSNColumnEnabled)
        {
            operationLSNIndex = builder.AddColumn(
                operationLSNColumnName,
                JET_coltypLongLong,
                sizeof(::FABRIC_SEQUENCE_NUMBER),
                JET_bitColumnFixed | JET_bitColumnNotNULL,
                &initialOperationLSN,
                sizeof(initialOperationLSN));
        }

        auto lastModifiedIndex = builder.AddColumn<JET_coltypLongLong>(
            lastModifiedColumnName,
            JET_bitColumnFixed | JET_bitColumnNotNULL);

        int lastModifiedOnPrimaryIndex = 0;
        if (this->IsLastModifiedOnPrimaryColumnEnabled)
        {
            lastModifiedOnPrimaryIndex = builder.AddColumn(
                lastModifiedOnPrimaryColumnName,
                JET_coltypLongLong,
                sizeof(_int64),
                JET_bitColumnFixed | JET_bitColumnNotNULL,
                const_cast<_int64 *>(&lastModifiedOnPrimaryDefVal),
                sizeof(lastModifiedOnPrimaryDefVal));
        }

        auto typeIndex = builder.AddColumn<JET_coltypLongBinary>(
            typeColumnName,
            maxTypeLengthCharacters * sizeof(wchar_t),
            JET_bitColumnTagged | JET_bitColumnNotNULL);

        auto keyIndex = builder.AddColumn<JET_coltypLongBinary>(
            keyColumnName,
            maxKeyLengthCharacters * sizeof(wchar_t),
            JET_bitColumnTagged | JET_bitColumnNotNULL);

        auto valueIndex = builder.AddColumn<JET_coltypLongBinary>(
            valueColumnName,
            maxValueLengthBytes,
            JET_bitColumnTagged | JET_bitColumnNotNULL);

        size_t keyMost = 0;
        jetErr = instance_->GetSystemParameter(JET_paramKeyMost, keyMost);

        if (JET_errSuccess == jetErr)
        {
            builder.AddIndex(
                primaryIndexName,
                primaryIndexKeys,
                JET_bitIndexPrimary | JET_bitIndexDisallowNull | JET_bitIndexKeyMost | JET_bitIndexDisallowTruncation,
                keyMost);

            if (this->IsOperationLSNColumnEnabled)
            {
                builder.AddIndex(
                    operationLSNIndexName,
                    operationLSNIndexKeys,
                    JET_bitIndexDisallowNull | JET_bitIndexKeyMost,
                    keyMost);
            }

            EseCursor cursor(database_, session_);
            jetErr = cursor.InitializeCreateTable(builder);

            if (JET_errSuccess == jetErr)
            {
                sequenceNumberColumnId_ = builder.GetColumnId(sequenceNumberIndex);

                if (this->IsOperationLSNColumnEnabled)
                {
                    operationLSNColumnId_ = builder.GetColumnId(operationLSNIndex);
                }

                lastModifiedColumnId_ = builder.GetColumnId(lastModifiedIndex);

                if (this->IsLastModifiedOnPrimaryColumnEnabled)
                {
                    lastModifiedOnPrimaryColumnId_ = builder.GetColumnId(lastModifiedOnPrimaryIndex);
                }
                                
                typeColumnId_ = builder.GetColumnId(typeIndex);
                keyColumnId_ = builder.GetColumnId(keyIndex);
                valueColumnId_ = builder.GetColumnId(valueIndex);

                EseStoreEventSource::Trace->StoreInitialized(traceId_, directory_, fileName_, true);
            }
        }

        return jetErr;
    }

    ErrorCode EseLocalStore::Cleanup()
    {
        auto error1 = Terminate();
        Drain();
        auto error2 = DeleteDbDirectories();

        return ErrorCode::FirstError(error1, error2);
    }

    ErrorCode EseLocalStore::Terminate()
    {
        ErrorCode retError;
        {
            Common::AcquireWriteLock acquire(lock_);

            if (aborting_)
            {
                return retError;
            }

            aborting_ = true;
        }

        if (defragmenter_)
        {
            defragmenter_->Cancel();
        }

        // Call ese pool abort before instance abort since EseSession is owned by EsePool
        // but EseInstance keeps them in the activeSessions_ w/o controlling lifetime
        if (esePool_)
        {
            esePool_->Abort();
        }

        if (instance_)
        {
            retError = instance_->Close();
        }

        if (exclusiveFileLock_)
        {
            exclusiveFileLock_->Release();
        }

        return retError;
    }

    ::FABRIC_TRANSACTION_ISOLATION_LEVEL EseLocalStore::GetDefaultIsolationLevel()
    {
        return FABRIC_TRANSACTION_ISOLATION_LEVEL::FABRIC_TRANSACTION_ISOLATION_LEVEL_SNAPSHOT;
    }

    ErrorCode EseLocalStore::CreateTransaction(
        __out TransactionSPtr & txResultSPtr)
    {
        this->PerfCounters.RateOfTransactions.Increment();

        // Ensure that any potential re-use of a tx shared pointer does not
        // result in a re-entrant acquisition of lock_ when txResultSPtr
        // is set to the new tx below. This is because destruction of a 
        // tx also acquires lock_.
        //
        txResultSPtr.reset();

        if (esePool_ == nullptr && !aborting_)
        {
            AcquireWriteLock acquire(lock_);

            if (esePool_ == nullptr && !aborting_)
            {
                esePool_ = std::make_shared<EsePool>(
                    *this,
                    StoreConfig::GetConfig().EsePoolMinSize,
                    StoreConfig::GetConfig().EsePoolAdjustmentSize,
                    TimeSpan::FromSeconds(StoreConfig::GetConfig().EsePoolEvictionPeriodInSeconds));
            }
        }

        shared_ptr<EseLocalStore::Transaction> txSPtr;
        {
            AcquireReadLock acquire(lock_);
            if (!aborting_)
            {
                txSPtr = std::make_shared<EseLocalStore::Transaction>(*this, esePool_);
            }
        }

        if (txSPtr)
        {
            auto error = txSPtr->ErrorCode;
            if (error.IsSuccess())
            {
                txResultSPtr = move(txSPtr);
            }

            return error;
        }
        else
        {
            return ErrorCodeValue::ObjectClosed;
        }
    }

    void EseLocalStore::OnConstructTransaction() 
    { 
        ++transactionCount_; 
    }

    void EseLocalStore::OnDestructTransaction()
    {
        if (--transactionCount_ == 0)
        {
            auto snapEvent = allStoreObjectsDestructedSPtr_;
            snapEvent->Set();
        }
    }

    ErrorCode EseLocalStore::CreateEnumerationByPrimaryIndex(
        __in TransactionSPtr const & transaction,
        __in std::wstring const & typeStart,
        __in std::wstring const & keyStart,
        __out EnumerationSPtr & enumerationSPtr)
    {
        return this->CreateEnumerationByTypeAndKey(transaction, typeStart, keyStart, false, enumerationSPtr);
    }

    ErrorCode EseLocalStore::CreateEnumerationByTypeAndKey(
        __in TransactionSPtr const & transaction,
        __in std::wstring const & type,
        __in std::wstring const & keyStart,
        __out EnumerationSPtr & enumerationSPtr)
    {
        return this->CreateEnumerationByTypeAndKey(transaction, type, keyStart, true, enumerationSPtr);
    }

    ErrorCode EseLocalStore::CreateEnumerationByTypeAndKey(
        __in TransactionSPtr const & transaction,
        __in std::wstring const & type,
        __in std::wstring const & keyStart,
        bool strictType,
        __out EnumerationSPtr & enumerationSPtr)
    {
        if (aborting_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        auto castedTx = CastTransactionSPtr<EseLocalStore::Transaction>(transaction);

        if (!castedTx)
        {
            TRACE_ERROR_AND_TESTASSERT(
                Constants::StoreSource,
                "{0}: CreateEnumerationByTypeAndKey(): failed to cast ESE transaction",
                this->TraceId);

            return ErrorCodeValue::OperationFailed;
        }

        this->PerfCounters.RateOfEnumerations.Increment();

        enumerationSPtr = std::make_shared<EseLocalStore::KeyEnumeration>(*this, castedTx, type, keyStart, strictType);
        return ErrorCodeValue::Success;
    }

    Common::ErrorCode EseLocalStore::CreateEnumerationByOperationLSN(
        __in TransactionSPtr const & transaction,
        __in _int64 fromOperationNumber,
        __out EnumerationSPtr & enumerationSPtr)
    {
        if (!this->IsOperationLSNColumnEnabled) { return ErrorCodeValue::InvalidOperation; }

        if (aborting_)
        {
            return ErrorCodeValue::ObjectClosed;
        }

        auto castedTx = CastTransactionSPtr<EseLocalStore::Transaction>(transaction);

        if (!castedTx)
        {
            TRACE_ERROR_AND_TESTASSERT(
                Constants::StoreSource,
                "{0}: CreateEnumerationByOperationLSN(): failed to cast ESE transaction",
                this->TraceId);

            return ErrorCodeValue::OperationFailed;
        }

        this->PerfCounters.RateOfEnumerations.Increment();

        enumerationSPtr = std::make_shared<EseLocalStore::ChangeEnumeration>(*this, castedTx, fromOperationNumber);
        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::GetLastChangeOperationLSN(
        TransactionSPtr const & transaction,
        __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        if (!this->IsOperationLSNColumnEnabled) { return ErrorCodeValue::InvalidOperation; }
        
        EseLocalStore::Transaction & tx = *(reinterpret_cast<EseLocalStore::Transaction*>(transaction.get()));

        if (!tx.ErrorCode.IsSuccess())
        {
            EseStoreEventSource::Trace->TransactionError(this->TraceId, tx.TraceId, tx.ErrorCode.ReadValue());
            return tx.ErrorCode;
        }

        EseCursorSPtr cursor;
        auto error = tx.TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }
        
        JET_ERR jetErr = cursor->SetCurrentIndex(operationLSNIndexName);

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->MakeKey(maxOperationLSN_, JET_bitNewKey);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Seek(JET_bitSeekLE);
            switch (jetErr)
            {
            case JET_errSuccess:
            case JET_wrnSeekNotEqual:
            case JET_wrnUniqueKey:
                jetErr = JET_errSuccess;
                break;
            }
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->RetrieveColumn<::FABRIC_SEQUENCE_NUMBER>(operationLSNColumnId_, operationLSN);
        }
        else if (JET_errRecordNotFound == jetErr)
        {
            operationLSN = 0;
            jetErr = JET_errSuccess;
        }

        // reset index to the default (primary index) so that subsequent
        // insert/update/delete operations within the same transaction work
        //
        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetCurrentIndex(primaryIndexName);
        }

        return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
    }

    ErrorCode EseLocalStore::EstimateRowCount(__out size_t & estimatedCount)
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        auto castedTx = CastTransactionSPtr<EseLocalStore::Transaction>(txSPtr);

        EseSessionSPtr session;
        error = castedTx->TryGetSession(session);
        if (!error.IsSuccess()) { return error; }

        EseCursorSPtr cursor;
        error = castedTx->TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }

        auto jetErr = CALL_ESE_NOTHROW(JetMove(
            session->SessionId,
            cursor->TableId,
            JET_MoveLast,
            0));
        if (JET_errSuccess != jetErr) { return this->JetToErrorCode(jetErr); }

        JET_RECPOS recPos = {0};
        jetErr = CALL_ESE_NOTHROW(JetGetRecordPosition(
            session->SessionId,
            cursor->TableId,
            &recPos,
            sizeof(recPos)));
        if (JET_errSuccess != jetErr) { return this->JetToErrorCode(jetErr); }

        estimatedCount = recPos.centriesTotal;

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::EstimateDbSizeBytes(__out size_t & estimatedDbSizeBytes)
    {
        TransactionSPtr txSPtr;
        auto error = this->CreateTransaction(txSPtr);
        if (!error.IsSuccess()) { return error; }

        auto castedTx = CastTransactionSPtr<EseLocalStore::Transaction>(txSPtr);

        EseSessionSPtr session;
        error = castedTx->TryGetSession(session);
        if (!error.IsSuccess()) { return error; }

        EseDatabaseSPtr database;
        error = castedTx->TryGetDatabase(database);
        if (!error.IsSuccess()) { return error; }

        auto sessionId = session->SessionId;
        auto dbId = database->DatabaseId;

        DWORD dbPageCount = 0;
        ULONG dbPageSize = 0;

        auto jetError = CALL_ESE_NOTHROW(JetGetDatabaseInfo(
            sessionId,
            dbId,
            reinterpret_cast<void*>(&dbPageCount),
            sizeof(dbPageCount),
            JET_DbInfoFilesize));

        if (JET_errSuccess != jetError)
        {
            WriteInfo(
                Constants::StoreSource,
                "{0} failed to get page count: error={1}",
                this->TraceId,
                jetError);

            return this->JetToErrorCode(jetError);
        }

        jetError = CALL_ESE_NOTHROW(JetGetDatabaseInfo(
            sessionId,
            dbId,
            reinterpret_cast<void*>(&dbPageSize),
            sizeof(dbPageSize),
            JET_DbInfoPageSize));

        if (JET_errSuccess != jetError)
        {
            WriteInfo(
                Constants::StoreSource,
                "{0} failed to get page size: error={1}",
                this->TraceId,
                jetError);

            return this->JetToErrorCode(jetError);
        }

        estimatedDbSizeBytes = static_cast<size_t>(dbPageCount) * dbPageSize;

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::GetOpenFileSize(__out int64 & fileSize)
    {
        if (instance_)
        {
            return instance_->GetOpenFileSize(fileSize);
        }
        else
        {
            return File::GetSize(filePath_, fileSize);
        }
    }

    ErrorCode EseLocalStore::Insert(
        TransactionSPtr const & transaction,
        std::wstring const & type,
        std::wstring const & key,
        __in void const * value,
        size_t valueSizeInBytes,
        _int64 sequenceNumber,
        FILETIME const * lastModifiedOnPrimaryUtc)
    {
        ASSERT_IF(sequenceNumber != ILocalStore::OperationNumberUnspecified && sequenceNumber < 0, "{0} Invalidate sequenceNumber {1} for EseLocalStore::Insert", this->TraceId, sequenceNumber);
        EseLocalStore::Transaction & tx = *(reinterpret_cast<EseLocalStore::Transaction*>(transaction.get()));
        EseStoreEventSource::Trace->Insert(
            this->TraceId,
            tx.TraceId,
            type,
            key,
            static_cast<uint64>(valueSizeInBytes),
            -1,
            -1);

        this->PerfCounters.RateOfInserts.Increment();

        if (!tx.ErrorCode.IsSuccess())
        {
            EseStoreEventSource::Trace->TransactionError(this->TraceId, tx.TraceId, tx.ErrorCode.ReadValue());
            return tx.ErrorCode;
        }

        this->PerfCounters.AvgSizeOfWritesBase.Increment();
        this->PerfCounters.AvgSizeOfWrites.IncrementBy(valueSizeInBytes);

        EseCursorSPtr cursor;
        auto error = tx.TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }

        JET_ERR jetErr = cursor->PrepareUpdate(JET_prepInsert);

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<_int64>(lastModifiedColumnId_, this->GetStoreUtcInt64());
        }

        if (JET_errSuccess == jetErr && this->IsLastModifiedOnPrimaryColumnEnabled && lastModifiedOnPrimaryUtc != nullptr)
        {
            jetErr = cursor->SetColumn<_int64>(lastModifiedOnPrimaryColumnId_, this->FILETIMEToInt64(lastModifiedOnPrimaryUtc));
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<std::wstring>(typeColumnId_, type);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<std::wstring>(keyColumnId_, key);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn(valueColumnId_, value, valueSizeInBytes);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<_int64>(sequenceNumberColumnId_, sequenceNumber == ILocalStore::OperationNumberUnspecified ? 0 : sequenceNumber);
        }

        if (JET_errSuccess == jetErr && this->IsOperationLSNColumnEnabled)
        {
            // We set operationLSN and sequence number to be the same value
            jetErr = cursor->SetColumn<_int64>(operationLSNColumnId_, sequenceNumber == ILocalStore::OperationNumberUnspecified ? 0 : sequenceNumber);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Update();
        }

        if (JET_errKeyDuplicate == jetErr)
        {
            // Undo prepare update to continue using transaction
            //
            auto cancelJetErr = cursor->PrepareUpdate(JET_prepCancel);

            if (JET_errSuccess == cancelJetErr)
            {
                // Do not cache StoreRecordAlreadyExists in transaction.
                // This allows us to implement a more efficient
                // pattern for TryInsert() re-using the same transaction.
                //
                return this->JetToErrorCode(jetErr);
            }
            else
            {
                return tx.SetEseError(cancelJetErr);
            }
        }
        else
        {
            return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
        }
    }

    ErrorCode EseLocalStore::Update(
        TransactionSPtr const & transaction,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber,
        std::wstring const & newKey,
        __in_opt void const * newValue,
        size_t valueSizeInBytes,
        _int64 sequenceNumber,
        FILETIME const * lastModifiedOnPrimaryUtc)
    {
        ASSERT_IF(sequenceNumber != ILocalStore::OperationNumberUnspecified && sequenceNumber < 0, "{0} Invalid sequenceNumber {1} for EseLocalStore::Update", this->TraceId, sequenceNumber);
        EseLocalStore::Transaction & tx = *(reinterpret_cast<EseLocalStore::Transaction*>(transaction.get()));
        EseStoreEventSource::Trace->Update(
            this->TraceId,
            tx.TraceId,
            type,
            key,
            checkSequenceNumber,
            newKey,
            static_cast<uint64>(valueSizeInBytes),
            -1,
            -1);

        this->PerfCounters.RateOfUpdates.Increment();

        if (!tx.ErrorCode.IsSuccess())
        {
            EseStoreEventSource::Trace->TransactionError(this->TraceId, tx.TraceId, tx.ErrorCode.ReadValue());
            return tx.ErrorCode;
        }

        this->PerfCounters.AvgSizeOfWritesBase.Increment();
        this->PerfCounters.AvgSizeOfWrites.IncrementBy(valueSizeInBytes);

        EseCursorSPtr cursor;
        auto error = tx.TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }

        JET_ERR jetErr = cursor->MakeKey(type, JET_bitNewKey);

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->MakeKey(key, JET_bitFullColumnStartLimit);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Seek(JET_bitSeekEQ);
        }

        if (JET_errSuccess == jetErr && ILocalStore::SequenceNumberIgnore != checkSequenceNumber)
        {
            _int64 currentSequenceNumber = 0;
            jetErr = this->GetV1CompatibleSequenceNumber(*cursor, currentSequenceNumber);

            if (JET_errSuccess == jetErr && checkSequenceNumber != currentSequenceNumber)
            {
                EseStoreEventSource::Trace->SequenceCheckMismatch(
                    this->TraceId, 
                    tx.TraceId, 
                    type, 
                    key, 
                    checkSequenceNumber, 
                    currentSequenceNumber);

                return tx.SetError(ErrorCodeValue::StoreSequenceNumberCheckFailed);
            }
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->PrepareUpdate(JET_prepReplace);
        }

        if (ILocalStore::OperationNumberUnspecified != sequenceNumber)
        {
            // We set operationLSN and sequence number to be the same value
            
            if (JET_errSuccess == jetErr)
            {
                jetErr = cursor->SetColumn<_int64>(sequenceNumberColumnId_, sequenceNumber);
            }

            if (JET_errSuccess == jetErr && this->IsOperationLSNColumnEnabled)
            {
                jetErr = cursor->SetColumn<_int64>(operationLSNColumnId_, sequenceNumber);
            }
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<_int64>(lastModifiedColumnId_, this->GetStoreUtcInt64());
        }

        if (JET_errSuccess == jetErr && this->IsLastModifiedOnPrimaryColumnEnabled && lastModifiedOnPrimaryUtc != nullptr)
        {
            jetErr = cursor->SetColumn<_int64>(lastModifiedOnPrimaryColumnId_, this->FILETIMEToInt64(lastModifiedOnPrimaryUtc));
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn(valueColumnId_, newValue, valueSizeInBytes);
        }

        if (JET_errSuccess == jetErr && newKey.size() > 0)
        {
            jetErr = cursor->SetColumn<std::wstring>(keyColumnId_, newKey);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Update();
        }

        if (JET_errRecordNotFound == jetErr)
        {
            // Do not cache StoreRecordNotFound in transaction.
            // This allows us to implement a more efficient
            // pattern for TryUpdate() re-using the same transaction.
            //
            // No need to undo PrepareUpdate since JET_errRecordNotFound comes
            // from cursor.Seek(JET_bitSeekEQ).
            //
            return this->JetToErrorCode(jetErr);
        }
        else
        {
            return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
        }
    }

    ErrorCode EseLocalStore::Delete(
        TransactionSPtr const & transaction,
        std::wstring const & type,
        std::wstring const & key,
        _int64 checkSequenceNumber)
    {
        EseLocalStore::Transaction & tx = *(reinterpret_cast<EseLocalStore::Transaction*>(transaction.get()));
        EseStoreEventSource::Trace->Delete(
            this->TraceId,
            tx.TraceId,
            type,
            key,
            checkSequenceNumber);

        this->PerfCounters.RateOfDeletes.Increment();

        if (!tx.ErrorCode.IsSuccess())
        {
            EseStoreEventSource::Trace->TransactionError(this->TraceId, tx.TraceId, tx.ErrorCode.ReadValue());
            return tx.ErrorCode;
        }

        EseCursorSPtr cursor;
        auto error = tx.TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }

        JET_ERR jetErr = cursor->MakeKey(type, JET_bitNewKey);
        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->MakeKey(key, JET_bitFullColumnStartLimit);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Seek(JET_bitSeekEQ);
        }

        if (JET_errSuccess == jetErr && ILocalStore::SequenceNumberIgnore != checkSequenceNumber)
        {
            _int64 currentSequenceNumber = 0;
            jetErr = this->GetV1CompatibleSequenceNumber(*cursor, currentSequenceNumber);

            if (JET_errSuccess == jetErr && checkSequenceNumber != currentSequenceNumber)
            {
                EseStoreEventSource::Trace->SequenceCheckMismatch(
                    this->TraceId, 
                    tx.TraceId, 
                    type, 
                    key, 
                    checkSequenceNumber, 
                    currentSequenceNumber);

                return ErrorCodeValue::StoreSequenceNumberCheckFailed;
            }
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Delete();
        }

        if (JET_errRecordNotFound == jetErr)
        {
            // Do not cache StoreRecordNotFound in transaction.
            // This allows us to implement a more efficient
            // pattern for TryDelete() re-using the same transaction.
            //
            return this->JetToErrorCode(jetErr);
        }
        else
        {
            return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
        }
    }

    ErrorCode EseLocalStore::GetOperationLSN(
        TransactionSPtr const & txSPtr,
        std::wstring const & type,
        std::wstring const & key,
        __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        if (!this->IsOperationLSNColumnEnabled) { return ErrorCodeValue::InvalidOperation; }

        EnumerationSPtr enumSPtr;
        auto error = this->CreateEnumerationByTypeAndKey(txSPtr, type, key, enumSPtr);

        if (error.IsSuccess())
        {
            error = enumSPtr->MoveNext();
        }

        if (error.IsSuccess())
        {
            wstring existingKey;
            error = enumSPtr->CurrentKey(existingKey);

            if (error.IsSuccess())
            {
                if (existingKey == key)
                {
                    error = enumSPtr->CurrentOperationLSN(operationLSN);
                }
                else
                {
                    error = ErrorCodeValue::StoreRecordNotFound;
                }
            }
        }

        return (error.IsError(ErrorCodeValue::EnumerationCompleted) ? ErrorCodeValue::StoreRecordNotFound : error);
    }

    ErrorCode EseLocalStore::UpdateOperationLSN(
        TransactionSPtr const & transaction,
        std::wstring const & type,
        std::wstring const & key,
        __in ::FABRIC_SEQUENCE_NUMBER operationLSN)
    {
        if (!this->IsOperationLSNColumnEnabled) { return ErrorCodeValue::InvalidOperation; }

        EseLocalStore::Transaction & tx = *(reinterpret_cast<EseLocalStore::Transaction*>(transaction.get()));
        EseStoreEventSource::Trace->UpdateOperationLSN(
            this->TraceId,
            tx.TraceId,
            type,
            key,
            operationLSN);

        if (!tx.ErrorCode.IsSuccess())
        {
            EseStoreEventSource::Trace->TransactionError(this->TraceId, tx.TraceId, tx.ErrorCode.ReadValue());
            return tx.ErrorCode;
        }

        EseCursorSPtr cursor;
        auto error = tx.TryGetUpdateCursor(cursor);
        if (!error.IsSuccess()) { return error; }

        JET_ERR jetErr = cursor->MakeKey(type, JET_bitNewKey);

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->MakeKey(key, JET_bitFullColumnStartLimit);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Seek(JET_bitSeekEQ);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->PrepareUpdate(JET_prepReplace);
        }

        if (JET_errSuccess == jetErr && operationLSN > 0)
        {
            ::FABRIC_SEQUENCE_NUMBER currentOperationLSN;

            jetErr = cursor->RetrieveColumn<::FABRIC_SEQUENCE_NUMBER>(operationLSNColumnId_, currentOperationLSN);

            if (JET_errSuccess == jetErr)
            {
                ASSERT_IF(operationLSN < currentOperationLSN, "Invalid operation LSN : current {0}, new {1}.", currentOperationLSN, operationLSN);

                if (operationLSN == currentOperationLSN)
                {
                    jetErr = cursor->PrepareUpdate(JET_prepCancel);

                    return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
                }
            }
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<::FABRIC_SEQUENCE_NUMBER>(operationLSNColumnId_, operationLSN);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->SetColumn<::FABRIC_SEQUENCE_NUMBER>(sequenceNumberColumnId_, operationLSN);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = cursor->Update();
        }

        return JET_errSuccess == jetErr ? ErrorCode::Success() : tx.SetEseError(jetErr);
    }     

    FILETIME EseLocalStore::GetStoreUtcFILETIME()
    {
        return DateTime::Now().AsFileTime;
    }

    _int64 EseLocalStore::GetStoreUtcInt64()
    {
        FILETIME nowUtc = DateTime::Now().AsFileTime;

        return this->FILETIMEToInt64(&nowUtc);
    }

    _int64 EseLocalStore::FILETIMEToInt64(FILETIME const * filetime)
    {
        if (filetime == nullptr)
        {
            return 0;
        }

        ULARGE_INTEGER convert;
        convert.LowPart = filetime->dwLowDateTime;
        convert.HighPart = filetime->dwHighDateTime;

        return static_cast<_int64>(convert.QuadPart);
    }

    FILETIME EseLocalStore::Int64ToFILETIME(_int64 const & time)
    {
        ULARGE_INTEGER convert;
        convert.QuadPart = static_cast<unsigned _int64>(time);

        FILETIME filetime;
        filetime.dwHighDateTime = convert.HighPart;
        filetime.dwLowDateTime = convert.LowPart;

        return filetime;
    }

    JET_ERR EseLocalStore::GetV1CompatibleSequenceNumber(__in EseCursor & cursor, __out _int64 & sequenceNumber)
    {
        // For backwards compatibility with V1, both columns are maintained to support rollback.
        //
        // In V1, the sequence number column increments independent of the LSN column. In V2, both columns are set
        // equal to the LSN. Conditional operations must get/check using the same column.
        //
        // In V1, local stores used directly (i.e. LFUM) did not have an LSN column.
        //
        if (this->IsOperationLSNColumnEnabled)
        {
            return  cursor.RetrieveColumn<_int64>(operationLSNColumnId_, sequenceNumber);
        }
        else
        {
            return  cursor.RetrieveColumn<_int64>(sequenceNumberColumnId_, sequenceNumber);
        }
    }

    // ------------------------------------------------------------------------
    // EseLocalStore::Transaction
    //

    EseLocalStore::Transaction::Transaction(EseLocalStore &store, EsePoolSPtr const esePool)
        : owner_(store),
        esePool_(esePool),
        esePoolItem_(),
        esePoolItemLock_(),
        error_(ErrorCodeValue::Success),
        active_(false),
        rolledback_(false),
        enumerationCount_(0),
        traceId_(++eseStoreObjectIds),
        lifetimeStopwatch_()
    {
        error_ = esePool_->GetItem(esePoolItem_);

        if (error_.IsSuccess())
        { 
            EseStoreEventSource::Trace->CreateTransaction(
                owner_.TraceId,
                this->TraceId,
                esePoolItem_->Session->SessionId);

            JET_ERR jetErr = esePoolItem_->Session->BeginTransaction();
            if (JET_errSuccess == jetErr)
            {
                active_.store(true);
            }
            else
            {
                // ignoring return value because it is captured in error_
                this->SetEseError(jetErr).ReadValue();
            }
        }
        else
        {
            EseStoreEventSource::Trace->CreateTransaction(
                owner_.TraceId,
                this->TraceId,
                0);
        }

        lifetimeStopwatch_.Start();

        owner_.OnConstructTransaction();
    }

    EseLocalStore::Transaction::Transaction(EseLocalStore &store, ErrorCodeValue::Enum const error)
        : owner_(store),
        esePool_(),
        esePoolItem_(),
        error_(error),
        active_(false),
        rolledback_(false),
        enumerationCount_(0),
        traceId_(++eseStoreObjectIds)
    {
        EseStoreEventSource::Trace->CreateTransaction(
            owner_.TraceId,
            this->TraceId,
            0);
        owner_.OnConstructTransaction();
    }

    EseLocalStore::Transaction::~Transaction()
    {
        if (enumerationCount_ != 0)
        {
            // Forcefully draining transactions (TransactionDrainTimeout > 0)
            // may release an ESE transaction while there are still outstanding
            // enumerations in the application.
            //
            wstring message = wformatString(
                "{0}: enumerations still exist using this ESE transaction: count = {1}",
                this->TraceId,
                enumerationCount_);

            WriteInfo(Constants::StoreSource, "{0}", message);
        }

        this->Rollback();

        if (esePoolItem_.get() != nullptr)
        {
            // Perform cleanup, but don't make pool item available for other transactions
            // if there are still outstanding enumerations, which may trigger multi-threaded
            // tx use errors if there's a racing read. The outstanding enumerations will eventually
            // release as well since the underlying transaction is gone.
            //
            esePool_->ReturnToPool(
                esePoolItem_, 
                !error_.IsSuccess() || (enumerationCount_ != 0));
        }

        lifetimeStopwatch_.Stop();

        owner_.PerfCounters.AvgTransactionLifetimeBase.Increment();
        owner_.PerfCounters.AvgTransactionLifetime.IncrementBy(lifetimeStopwatch_.ElapsedMilliseconds);

        owner_.OnDestructTransaction();
    }

    ErrorCode EseLocalStore::Transaction::Commit(
        ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
        Common::TimeSpan const timeout)
    {
        UNREFERENCED_PARAMETER(timeout);

        if (active_.exchange(false))
        {
            EseSessionSPtr session;
            auto error = this->TryGetSession(session);
            if (!error.IsSuccess()) { return error; }

            EseStoreEventSource::Trace->Commit(owner_.TraceId, this->TraceId, session->SessionId);

            this->owner_.PerfCounters.RateOfSyncCommits.Increment();

            if (error_.IsSuccess())
            {
                auto jetError = session->CommitTransaction();
                error_ = owner_.JetToErrorCode(jetError);
            }
            else
            {
                WriteInfo(Constants::StoreSource, "{0} cannot commit failed transaction: error={1}", this->TraceId, error_);

                session->RollbackTransaction();
            }
            
            if (enumerationCount_ == 0)
            {
                auto poolItem = this->TryTakePoolItem();
                if (poolItem.get() != nullptr)
                {
                    esePool_->ReturnToPool(poolItem, !error_.IsSuccess() /* deletePoolItem */);
                }
            }

            if (!error_.IsSuccess())
            {
                EseStoreEventSource::Trace->TransactionError(
                    this->LocalStore.TraceId,
                    this->TraceId,
                    error_.ReadValue());
            }
            // Return a default value since Ese local store does not implement commit sequence numbers.
            commitSequenceNumber = -1;
        }
        else if (error_.IsSuccess())
        {
            error_ = ErrorCodeValue::StoreTransactionNotActive;
        }

        return error_;
    }

    ErrorCode EseLocalStore::Transaction::TryGetPoolItem(__out EsePoolItemSPtr & poolItem)
    {
        AcquireReadLock lock(esePoolItemLock_);

        if (esePoolItem_.get() == nullptr) { return ErrorCodeValue::StoreTransactionNotActive; }

        poolItem = esePoolItem_;

        return ErrorCodeValue::Success;
    }

    EseLocalStore::EsePoolItemSPtr EseLocalStore::Transaction::TryTakePoolItem()
    {
        AcquireWriteLock lock(esePoolItemLock_);

        auto poolItem = esePoolItem_;

        esePoolItem_.reset();

        return poolItem;
    }

    ErrorCode EseLocalStore::Transaction::TryGetSession(__out EseSessionSPtr & session)
    {
        EsePoolItemSPtr poolItem;
        auto error = this->TryGetPoolItem(poolItem);
        if (!error.IsSuccess()) { return error; }

        session = poolItem->Session;

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::Transaction::TryGetDatabase(__out EseDatabaseSPtr & database)
    {
        EsePoolItemSPtr poolItem;
        auto error = this->TryGetPoolItem(poolItem);
        if (!error.IsSuccess()) { return error; }

        database = poolItem->Database;

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::Transaction::TryGetUpdateCursor(__out EseCursorSPtr & cursor)
    {
        EsePoolItemSPtr poolItem;
        auto error = this->TryGetPoolItem(poolItem);
        if (!error.IsSuccess()) { return error; }

        cursor = poolItem->UpdateCursor;

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::Transaction::TryCreateCursor(__out EseCursorSPtr & cursor)
    {
        EsePoolItemSPtr poolItem;
        auto error = this->TryGetPoolItem(poolItem);
        if (!error.IsSuccess()) { return error; }

        cursor = poolItem->CreateCursor();

        return ErrorCodeValue::Success;
    }

    Common::AsyncOperationSPtr EseLocalStore::Transaction::BeginCommit(
        Common::TimeSpan const timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & state)
    {
        bool wasActive = active_.exchange(false);

        EsePoolItemSPtr poolItem;
        auto error = this->TryGetPoolItem(poolItem);

        if (error.IsSuccess() && wasActive)
        {
            auto const & session = poolItem->Session;

            EseStoreEventSource::Trace->BeginCommit(owner_.TraceId, this->TraceId, session->SessionId);
                
            this->owner_.PerfCounters.RateOfAsyncCommits.Increment();

            return session->BeginCommitTransaction(this->owner_.PerfCounters, timeout, callback, state);
        }
        else
        {
            return AsyncOperation::CreateAndStart<EseCommitAsyncOperation>(
                error_.IsSuccess() ? ErrorCodeValue::StoreTransactionNotActive : error_,
                this->owner_.PerfCounters,
                callback,
                state);
        }
    }

    Common::ErrorCode EseLocalStore::Transaction::EndCommit(
        Common::AsyncOperationSPtr const & operation,
        ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
    {
        // Return a default value since Ese local store does not implement commit sequencer numbers.
        commitSequenceNumber = ILocalStore::SequenceNumberIgnore;

        return EseCommitAsyncOperation::End(operation);
    }

    void EseLocalStore::Transaction::Rollback()
    {
        error_.ReadValue();

        if (active_.exchange(false))
        {
            auto poolItem = this->TryTakePoolItem();
            if (poolItem.get() == nullptr) { return; }

            EseStoreEventSource::Trace->Rollback(
                this->LocalStore.TraceId,
                this->TraceId,
                poolItem->Session->SessionId);

            this->owner_.PerfCounters.RateOfRollbacks.Increment();

            JET_ERR jetErr = poolItem->Session->RollbackTransaction();

            rolledback_ = true;

            if (enumerationCount_ == 0)
            {
                esePool_->ReturnToPool(poolItem, JET_errSuccess != jetErr /* deletePoolItem */);
            }
        }
    }

    ErrorCode EseLocalStore::Transaction::SetEseError(JET_ERR error)
    {
        this->error_ = owner_.JetToErrorCode(error);
        return this->error_;
    }

    // ------------------------------------------------------------------------
    // EseLocalStore::Enumeration
    //

    EseLocalStore::Enumeration::Enumeration(
        EseLocalStore & store,
        EseStoreTransactionSPtr const & transaction)
        : owner_(store),
          txWPtr_(transaction),
          cursor_(),
          error_(),
          offset_(-1),
          traceId_(++eseStoreObjectIds),
          txTraceId_()
    {
        auto txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            if (error_.IsSuccess())
            {
                error_ = txSPtr->TryCreateCursor(cursor_);
            }

            if (error_.IsSuccess())
            {
                txTraceId_ = txSPtr->TraceId;

                txSPtr->OnConstructEnumeration();
            }
        }
        else
        {
            error_ = ErrorCodeValue::StoreTransactionNotActive;
        }
    }

    EseLocalStore::Enumeration::Enumeration(
        EseLocalStore & store,
        EseStoreTransactionSPtr const & transaction,
        ErrorCodeValue::Enum const error)
        : owner_(store),
          txWPtr_(transaction),
          cursor_(),
          error_(error),
          offset_(-1),
          traceId_(++eseStoreObjectIds)
    {
        auto txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            txTraceId_ = txSPtr->TraceId;

            txSPtr->OnConstructEnumeration();
        }
    }

    EseLocalStore::Enumeration::~Enumeration()
    {
        auto txSPtr = txWPtr_.lock();
        if (txSPtr)
        {
            if (txSPtr->Rolledback && cursor_)
            {
                cursor_->MarkTableClosedOnTxRollback();
            }

            EseStoreEventSource::Trace->EnumerationDestructing(
                txSPtr->LocalStore.TraceId,
                txSPtr->TraceId,
                traceId_,
                offset_ + 1, // offset_ starts at -1
                error_.IsError(ErrorCodeValue::EnumerationCompleted));

            txSPtr->OnDestructEnumeration();
        }
        else if (cursor_)
        {
            cursor_->MarkTableClosedOnTxRollback();
        }
    }

    ErrorCode EseLocalStore::Enumeration::TryGetTxSPtr(__out EseStoreTransactionSPtr & txSPtr)
    {
        txSPtr = txWPtr_.lock();

        if (txSPtr.get() != nullptr && txSPtr->IsActive) 
        { 
            return ErrorCodeValue::Success;
        }
        else
        {
            WriteInfo(
                Constants::StoreSource,
                "{0}: MoveNext(): transaction not active",
                this->TraceId);

            return ErrorCodeValue::StoreTransactionNotActive; 
        }
    }

    ErrorCode EseLocalStore::Enumeration::MoveNext()
    {
        EseStoreTransactionSPtr unusedTxSptr;
        return this->MoveNext(unusedTxSptr);
    }

    ErrorCode EseLocalStore::Enumeration::MoveNext(__out EseStoreTransactionSPtr & txSPtr)
    {
        if (!error_.IsSuccess()) { return error_; }

        error_ = this->TryGetTxSPtr(txSPtr);
        if (!error_.IsSuccess()) { return error_; }

        JET_ERR jetErr = JET_errSuccess;

        if (++offset_ == 0)
        {
            jetErr = cursor_->InitializeOpenTable(Constants::TableName);

            if (JET_errSuccess == jetErr)
            {
                jetErr = this->InitializeCursorAndSeekFirstItem();

                switch (jetErr)
                {
                case JET_errSuccess:
                case JET_wrnSeekNotEqual:
                case JET_wrnUniqueKey:
                    jetErr = JET_errSuccess;
                    break;
                case JET_errRecordNotFound:
                    return this->SetError(ErrorCodeValue::EnumerationCompleted);
                default:
                    return this->SetEseError(jetErr);
                }
            }
        }
        else
        {
            jetErr = cursor_->Move(JET_MoveNext);
            switch (jetErr)
            {
            case JET_errSuccess:
                break;
            case JET_errNoCurrentRecord:
                return this->SetError(ErrorCodeValue::EnumerationCompleted);
            default:
                return this->SetEseError(jetErr);
            }
        }

        if (JET_errSuccess != jetErr)
        {
            return this->SetEseError(jetErr);
        }

        return error_;
    }

    // The only error possible for RetrieveColumn that does not represent a coding error for us
    // once we get here is JET_errInstanceUnavailable, which represents a coding error for ESE.  In
    // either case let the Exception go so we fail-fast.
    //
    // See http://msdn.microsoft.com/en-us/library/ms684054(v=EXCHG.10).aspx for the list of error
    // codes.

    ErrorCode EseLocalStore::Enumeration::CurrentOperationLSN(_int64 & operationNumber)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        return owner_.JetToErrorCode(this->LocalStore.GetV1CompatibleSequenceNumber(*cursor_, operationNumber));
    }

    ErrorCode EseLocalStore::Enumeration::CurrentLastModifiedFILETIME(FILETIME & fileTime)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        _int64 time;
        JET_ERR jetErr = cursor_->RetrieveColumn<_int64>(this->LocalStore.lastModifiedColumnId_, time);

        if (JET_errSuccess == jetErr)
        {
            fileTime = this->LocalStore.Int64ToFILETIME(time);
        }

        return owner_.JetToErrorCode(jetErr);
    }

    ErrorCode EseLocalStore::Enumeration::CurrentLastModifiedOnPrimaryFILETIME(FILETIME & fileTime)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        _int64 time;
        JET_ERR jetErr = cursor_->RetrieveColumn<_int64>(this->LocalStore.lastModifiedOnPrimaryColumnId_, time);

        if (JET_errSuccess == jetErr)
        {
            fileTime = this->LocalStore.Int64ToFILETIME(time);
        }

        return owner_.JetToErrorCode(jetErr);
    }

    ErrorCode EseLocalStore::Enumeration::CurrentType(
        std::wstring & buffer)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        this->owner_.PerfCounters.RateOfTypeReads.Increment();

        return owner_.JetToErrorCode(cursor_->RetrieveColumn<std::wstring>(this->LocalStore.typeColumnId_, buffer));
    }

    ErrorCode EseLocalStore::Enumeration::CurrentKey(
        std::wstring & buffer)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }
        
        this->owner_.PerfCounters.RateOfKeyReads.Increment();

        return owner_.JetToErrorCode(cursor_->RetrieveColumn<std::wstring>(this->LocalStore.keyColumnId_, buffer));
    }

    ErrorCode EseLocalStore::Enumeration::CurrentValue(
        std::vector<unsigned _int8> & value)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        this->owner_.PerfCounters.RateOfValueReads.Increment();
        
        error = owner_.JetToErrorCode(cursor_->RetrieveColumn<std::vector<unsigned _int8>>(this->LocalStore.valueColumnId_, /*out*/value));

        if (error.IsSuccess())
        {
            this->owner_.PerfCounters.AvgSizeOfReadsBase.Increment();
            this->owner_.PerfCounters.AvgSizeOfReads.IncrementBy(value.size());
        }

        return error;
    }

    ErrorCode EseLocalStore::Enumeration::CurrentValueSize(
        size_t & size)
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->CheckState(txSPtr);
        if (!error.IsSuccess()) { return error; }

        return owner_.JetToErrorCode(cursor_->RetrieveActualSize(this->LocalStore.valueColumnId_, /*out*/size));
    }

    ErrorCode EseLocalStore::Enumeration::CheckState(__out EseStoreTransactionSPtr & txSPtr)
    {
        if (offset_ < 0)
        {
            TRACE_ERROR_AND_TESTASSERT(
                Constants::StoreSource,
                "{0}: MoveNext() not called yet",
                this->TraceId);

            return ErrorCodeValue::InvalidState;
        }

        if (!error_.IsSuccess())
        {
            WriteInfo(
                Constants::StoreSource,
                "{0}: Enumeration already failed with {1} - enumeration not active",
                this->TraceId,
                error_);

            return error_;
        }

        error_ = this->TryGetTxSPtr(txSPtr);

        return error_;
    }

    EseLocalStore::KeyEnumeration::KeyEnumeration(
        EseLocalStore & store,
        EseStoreTransactionSPtr const & transaction,
        std::wstring const & type,
        std::wstring const & keyStart,
        bool strictType)
        : Enumeration(store, transaction),
          type_(type),
          keyStart_(keyStart),
          buffer_(L""),
          strictType_(strictType)
    {
        EseStoreEventSource::Trace->CreateEnumeration(
            this->LocalStore.TraceId,
            this->TransactionTraceId,
            this->TraceId,
            type_,
            keyStart_,
            strictType_);
    }

    // Called from EseLocalStore::Enumeration::MoveNext() - Tx is already refcounted
    //
    JET_ERR EseLocalStore::KeyEnumeration::InitializeCursorAndSeekFirstItem()
    {
        JET_ERR jetError = this->Cursor.SetCurrentIndex(primaryIndexName);

        if (JET_errSuccess == jetError)
        {
            jetError = this->Cursor.MakeKey(type_, JET_bitNewKey);
        }

        if (JET_errSuccess == jetError)
        {
            jetError = this->Cursor.MakeKey(keyStart_, JET_bitFullColumnStartLimit);
        }
		
		if (JET_errSuccess == jetError)
        {
            jetError = this->Cursor.PrepareForScan();
        }

        if (JET_errSuccess == jetError)
        {
            jetError = this->Cursor.Seek(JET_bitSeekGE);
        }

        return jetError;
    }

    Common::ErrorCode EseLocalStore::KeyEnumeration::MoveNext()
    {
        EseStoreTransactionSPtr txSPtr;
        auto error = this->Enumeration::MoveNext(txSPtr);

        if (error.IsSuccess())
        {
            JET_ERR jetErr = this->Cursor.RetrieveColumn<std::wstring>(this->LocalStore.typeColumnId_, buffer_);

            if (JET_errSuccess != jetErr)
            {
                return this->SetError(this->LocalStore.JetToErrorCode(jetErr));
            }

            if (strictType_ && buffer_ != type_)
            {
                return this->SetError(ErrorCode(ErrorCodeValue::EnumerationCompleted));
            }
        }

        return error;
    }

    EseLocalStore::ChangeEnumeration::ChangeEnumeration(
        EseLocalStore & store,
        EseStoreTransactionSPtr const & transaction,
        ::FABRIC_SEQUENCE_NUMBER fromOperationLSN)
        : Enumeration(store, transaction),
          fromOperationLSN_(fromOperationLSN)
    {
        EseStoreEventSource::Trace->CreateChangeEnumeration(
            this->LocalStore.TraceId,
            this->TransactionTraceId,
            this->TraceId,
            fromOperationLSN);
    }

    // Called from EseLocalStore::Enumeration::MoveNext() - Tx is already refcounted
    //
    JET_ERR EseLocalStore::ChangeEnumeration::InitializeCursorAndSeekFirstItem()
    {
        JET_ERR jetErr = this->Cursor.SetCurrentIndex(operationLSNIndexName);
        if (JET_errSuccess == jetErr)
        {
            jetErr = this->Cursor.MakeKey(fromOperationLSN_, JET_bitNewKey);
        }

        if (JET_errSuccess == jetErr)
        {
            jetErr = this->Cursor.Seek(JET_bitSeekGE);
        }

        return jetErr;
    }

    EseLocalStore::EsePoolItem::EsePoolItem(EseLocalStore &store)
        : owner_(store),
        session_(EseSession::CreateSPtr(store.instance_)),
        database_(EseDatabase::CreateSPtr(session_)),
        cursor_(make_shared<EseCursor>(database_, session_))
    {
    }

    EseLocalStore::EsePoolItem::~EsePoolItem()
    {
    } 

    EseLocalStore::EsePoolItemSPtr EseLocalStore::EsePoolItem::CreateSPtr(EseLocalStore & store)
    {
        return shared_ptr<EsePoolItem>(new EsePoolItem(store));
    }

    JET_ERR EseLocalStore::EsePoolItem::InitializeForUse()
    {
        JET_ERR jetError = session_->Initialize();

        if (JET_errSuccess == jetError)
        {
            jetError = database_->InitializeOpen(owner_.filePath_);
        }

        if (JET_errSuccess == jetError)
        {
            jetError = cursor_->InitializeOpenTable(Constants::TableName);
        }

        if (JET_errSuccess == jetError)
        {
            jetError = cursor_->SetCurrentIndex(primaryIndexName);
        }

        return jetError;
    }

    std::shared_ptr<EseCursor> EseLocalStore::EsePoolItem::CreateCursor() const
    {
        return make_shared<EseCursor>(database_, session_);
    }

    EseLocalStore::EsePool::EsePool(
        EseLocalStore & store,
        uint minSize,
        uint adjustmentSize,
        Common::TimeSpan evictionPeriod)
        : minSize_(minSize),
        adjustmentSize_(adjustmentSize),
        evictionPeriodInTicks_(evictionPeriod.get_Ticks()),
        lastPeakUseTimeTickcount_(),
        poolGrowthPending_(false),
        store_(store),
        lock_(),
        inUsePoolItems_(),
        notInUsePoolItems_(),
        aborting_(false)
    {
        Grow(minSize_);
    }

    EseLocalStore::EsePool::~EsePool()
    {
    }

    void EseLocalStore::EsePool::Abort()
    {
        
        Common::AcquireWriteLock acquire(lock_);
        aborting_ = true;
    }

    // Grow the pool to the requested size
    void EseLocalStore::EsePool::Grow(uint adjustmentSize)
    {
        std::vector<EsePoolItemSPtr> newPoolItems;

        // Create the pool items, and initialize them w/o taking the lock
        while(newPoolItems.size() < adjustmentSize)
        {
            auto esePoolItem = EsePoolItem::CreateSPtr(store_);

            if (esePoolItem->InitializeForUse() == JET_errSuccess)
            {
                newPoolItems.push_back(esePoolItem);
            }
            else
            {
                break;
            }
        }

        Stopwatch stopwatch;
        stopwatch.Start();

        Common::AcquireWriteLock acquire(lock_);

        // Now add them to the pool taking the lock
        if (newPoolItems.size() > 0)
        {
            for (auto const & newPoolItem : newPoolItems)
            {
                auto result = notInUsePoolItems_.insert(make_pair(newPoolItem->Session->SessionId, newPoolItem));

                if (!result.second)
                {
                    TRACE_ERROR_AND_TESTASSERT(
                        Constants::StoreSource,
                        "New ESE pool item already exists: session={0}",
                        newPoolItem->Session->SessionId);
                }
            }

            // Mark the pooled items as recently used to prevent eviction of the newly created items right away
            int64 currentTimeInTicks = DateTime::Now().get_Ticks();

            for (uint index = (uint)inUsePoolItems_.size() + 1 ; index <= (uint)(inUsePoolItems_.size() + notInUsePoolItems_.size()) ; ++index)
            {
                lastPeakUseTimeTickcount_[index] = currentTimeInTicks;
            }

            stopwatch.Stop();

            WriteInfo(
                Constants::StoreSource, 
                "{0} EsePool grew: busy={1} free={2} elapsed={3}", 
                store_.TraceId,
                inUsePoolItems_.size(),
                notInUsePoolItems_.size(),
                stopwatch.Elapsed);
        }

        // Reset the event to indicate pool growth has completed
        poolGrowthPending_ = false;
    }

    // Shrink the pool by the requested size
    void EseLocalStore::EsePool::Shrink(uint shrinkCount)
    {
        // We need at least adjustmentSize_ always available to be used in the notInUsePoolItems
        // The pool should never shrink below minSize_
        ASSERT_IFNOT(
            (uint)notInUsePoolItems_.size() >= shrinkCount + adjustmentSize_ && (uint)(inUsePoolItems_.size() + notInUsePoolItems_.size()) >= minSize_ + shrinkCount,
            "Pool can not be shrunk to the requested size.");

        while (shrinkCount > 0)
        {
            notInUsePoolItems_.erase(notInUsePoolItems_.begin());
            --shrinkCount;
        }

        WriteInfo(
            Constants::StoreSource, 
            "{0} EsePool shrunk: busy={1} free={2}.", 
            store_.TraceId, 
            inUsePoolItems_.size(),
            notInUsePoolItems_.size());
    }

    // Get a free item for use
    Common::ErrorCode EseLocalStore::EsePool::GetItem(EsePoolItemSPtr & esePoolItem)
    {
        Common::ErrorCode error;
        Common::AcquireWriteLock acquire(lock_);

        // If there is no pool item never wait. We pre-create some using threadpool.
        if (notInUsePoolItems_.size() > 0)
        {
            auto unusedItem = notInUsePoolItems_.begin()->second;

            notInUsePoolItems_.erase(notInUsePoolItems_.begin());

            auto result = inUsePoolItems_.insert(make_pair(unusedItem->Session->SessionId, unusedItem));

            if (!result.second)
            {
                TRACE_ERROR_AND_TESTASSERT(
                    Constants::StoreSource,
                    "Free ESE pool item already in use: session={0}",
                    unusedItem->Session->SessionId);

                return ErrorCodeValue::StoreUnexpectedError;
            }

            esePoolItem = unusedItem;
        }
        else
        {
            auto newEsePoolItem = EsePoolItem::CreateSPtr(store_);

            JET_ERR eseError = newEsePoolItem->InitializeForUse();
                
            if (eseError == JET_errSuccess)
            {
                auto result = inUsePoolItems_.insert(make_pair(newEsePoolItem->Session->SessionId, newEsePoolItem));

                if (!result.second)
                {
                    TRACE_ERROR_AND_TESTASSERT(
                        Constants::StoreSource,
                        "ESE pool item already exists: session={0}",
                        newEsePoolItem->Session->SessionId);

                    return ErrorCodeValue::StoreUnexpectedError;
                }

                esePoolItem = newEsePoolItem;
            }
            else
            {
                error = store_.JetToErrorCode(eseError);
            }
        }

        // If available pool item count has fallen below adjustmentSize_ / 2 queue
        // up work to grow the pool on the thread pool
        if (!poolGrowthPending_ && notInUsePoolItems_.size() < adjustmentSize_)
        {
            auto thisSPtr = shared_from_this();
            poolGrowthPending_ = true;

            uint adjustmentSize = this->adjustmentSize_;
            Threadpool::Post([thisSPtr, adjustmentSize]() { thisSPtr->Grow(adjustmentSize); });
        }

        return error;
    }

    void EseLocalStore::EsePool::ReturnToPool(EsePoolItemSPtr const & esePoolItem, bool deletePoolItem)
    {
#if DBG
        store_.instance_->AssertSessionIsNotActive(*(esePoolItem->Session));
#endif
        
        if (!esePoolItem->Session->TryWaitForInvalidSessionThread())
        {
            TRACE_ERROR_AND_TESTASSERT(
                Constants::StoreSource, 
                "Current thread id {0} is not invalid thread id.", 
                esePoolItem->Session->currentThread_);
        }

        Common::AcquireWriteLock acquire(lock_);

        // Set last use time to current time for current in-use-pool-item count
        int64 currentTimeInTicks = DateTime::Now().get_Ticks();
        lastPeakUseTimeTickcount_[(uint)inUsePoolItems_.size()] = currentTimeInTicks;

        inUsePoolItems_.erase(esePoolItem->Session->SessionId);

        // If aborting_ is set EseInstance will walk through the list of active sessions and call abort on them.
        // We have to ensure the session remains alive
        if (aborting_ || (!deletePoolItem && !esePoolItem->Session->HasUnrecoverableError))
        {
            auto result = notInUsePoolItems_.insert(make_pair(esePoolItem->Session->SessionId, esePoolItem));

            if (!result.second)
            {
                TRACE_ERROR_AND_TESTASSERT(Constants::StoreSource, "Free ESE pool item already exists: session={0}.", esePoolItem->Session->SessionId);
            }
        }
        else
        {
            WriteInfo(Constants::StoreSource, "{0} EsePool deleting pool item {1}.", store_.TraceId, esePoolItem->Session->SessionId);
        }

        // Check and shrink if required
        uint notInUseItemsToKeep = (uint)notInUsePoolItems_.size();
        std::map<uint, int64>::iterator iter = lastPeakUseTimeTickcount_.find((uint)inUsePoolItems_.size() + notInUseItemsToKeep);

        // Pool should always have minSize_items, and a minimum of adjustmentSize_ should be available for use
        while (!aborting_ && notInUseItemsToKeep > adjustmentSize_ && inUsePoolItems_.size() + notInUseItemsToKeep > minSize_ )
        {
            // If last use time is within eviction period we have to keep this many items
            if (iter != lastPeakUseTimeTickcount_.end() && currentTimeInTicks < (iter->second + evictionPeriodInTicks_))
            {
                break;
            }

            --notInUseItemsToKeep;
            if (iter != lastPeakUseTimeTickcount_.end())
            {
                --iter;
            }
            else
            {
                iter = lastPeakUseTimeTickcount_.find((uint)inUsePoolItems_.size() + notInUseItemsToKeep);
            }
        }

        if (notInUsePoolItems_.size() > notInUseItemsToKeep)
        {
            Shrink((uint)notInUsePoolItems_.size() - notInUseItemsToKeep);
        }
    }

    ErrorCode EseLocalStore::CreateDbDirectory()
    {
        if (!Directory::Exists(directory_))
        {
            WriteInfo(Constants::StoreSource, "{0} creating new database directory: {1}", this->TraceId, directory_);
            auto error = Directory::Create2(directory_);
            if (!error.IsSuccess())
            {
                WriteWarning(
                    Constants::StoreSource,
                    "{0} couldn't create directory '{1}'.  error = {2}",
                    this->TraceId,
                    directory_, 
                    error);

                if (error.IsError(ErrorCodeValue::InvalidDirectory))
                {
                    error = ErrorCode(ErrorCodeValue::PathTooLong, wformatString(GET_STORE_RC(Path_Too_Long), directory_));
                }

                return error;
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode EseLocalStore::DeleteDbDirectories()
    {   
        auto error1 = DeleteDirectoryWithRetries(directory_);
        auto error2 = DeleteDirectoryWithRetries(EseInstance::GetRestoreDirectoryName(directory_));
        auto error3 = DeleteDirectoryWithRetries(EseInstance::GetLocalBackupDirectoryName(directory_));

        return ErrorCode::FirstError(ErrorCode::FirstError(error1, error2), error3); 
    }

    ErrorCode EseLocalStore::DeleteDirectoryWithRetries(wstring const & dbDirectory)
    {
        return EseLocalStoreUtility::DeleteDirectoryWithRetries(dbDirectory, this->TraceId);
    }
    
    /// <summary>
    /// Backs up the local store to the specified directory.
    /// </summary>
    /// <param name="dir">The destination directory where the current local store should be backed up to.</param>
    /// <param name="backupOption">The backup option. The default option is StoreBackupOption::Full.</param>
    /// <returns>ErrorCode::Success() if the backup was successfully done. An appropriate ErrorCode otherwise.</returns>
    ErrorCode EseLocalStore::Backup(wstring const & dir, StoreBackupOption::Enum backupOption)
    {
        LONG jetBackupOptions; 
        
        ErrorCode error = GetJetBackupOptions(backupOption, jetBackupOptions);

        if (!error.IsSuccess())
        {
            return error;
        }
        
        if (backupOption == StoreBackupOption::TruncateLogsOnly && !dir.empty())
        {
            error = ErrorCodeValue::InvalidArgument;
            WriteError(
                Constants::StoreSource,
                "{0} Folder path '{1}' should be empty for backup option {2}. Error: {3}",
                this->TraceId,
                dir,                
                backupOption,
                error);

            return error;
        }

        size_t dbSizeBytes = 0;
        error = this->EstimateDbSizeBytes(dbSizeBytes);

        if (!error.IsSuccess())
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} failed to estimate DB size: error={1}",
                this->TraceId,
                error);

            return error;
        }

        WriteInfo(
            Constants::StoreSource,
            "{0} Creating backup in '{1}'",
            this->TraceId,
            dir);

        // for rough analytics on backup times to determine good backup frequencies, 
        // no. of incremental backups to create before a full backup etc.
        Stopwatch sw;
        
        sw.Start();
        auto jetError = instance_->Backup(dir, jetBackupOptions);
        sw.Stop();

        if (jetError == JET_errSuccess)
        {
            WriteInfo(
                Constants::StoreSource,
                "{0} {1} backup succeeded: {2} bytes, {3} ms",
                this->TraceId,
                backupOption,                
                dbSizeBytes,
                sw.ElapsedMilliseconds); 
        }
        else
        {
            WriteInfo(
                Constants::StoreSource,
                "{0} {1} backup failed: {2}",
                this->TraceId,
                backupOption,
                jetError);
        }
        
        return this->JetToErrorCode(jetError);
    }

    ErrorCode EseLocalStore::PrepareRestoreForValidation(
        wstring const & dir,
        wstring const & instanceName)
    {
        if (instance_)
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} Cannot validate restore against existing instance",
                this->TraceId);
            return ErrorCodeValue::InvalidOperation;
        }

        directory_ = EseLocalStore::GetValidationDirectoryName(directory_);

        this->ConstructorInitialize();

        auto restoreDirectory = EseInstance::GetRestoreDirectoryName(directory_);

        auto error = Directory::Copy(dir, restoreDirectory, true);

        if (error.IsSuccess())
        {
            error = this->InnerInitialize(instanceName, true);

            if (error.IsSuccess())
            {
                WriteInfo(
                    Constants::StoreSource,
                    "{0} Initialization completed '{1}' for restore validation",
                    this->TraceId,
                    directory_);
            }
            else
            {
                WriteWarning(
                    Constants::StoreSource,
                    "{0} Failed to initialize '{1}' for restore validation: {2}",
                    this->TraceId,
                    directory_,
                    error);
            }
        }
        else
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} Failed to copy from '{1}' to '{2}' for restore validation: {3}",
                this->TraceId,
                dir, 
                restoreDirectory,
                error);
        }

        if (!error.IsSuccess())
        {
            // Best-effort cleanup
            auto innerError = this->DeleteDbDirectories();
            if (!innerError.IsSuccess())
            {
                WriteWarning(
                    Constants::StoreSource,
                    "{0} Failed to cleanup database directories: {1}",
                    this->TraceId,
                    innerError);
            }
        }

        return error;
    }

    ErrorCode EseLocalStore::PrepareRestoreFromValidation()
    {
        auto validationDirectory = EseLocalStore::GetValidationDirectoryName(directory_);
        auto srcRestoreDirectory = EseInstance::GetRestoreDirectoryName(validationDirectory);

        auto restoreError = this->PrepareRestore(srcRestoreDirectory);

        auto innerError = Directory::Delete(validationDirectory, true);
        if (!innerError.IsSuccess())
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} Failed to delete validation directory '{1}' : {2}. Please delete this directory manually.",
                this->TraceId,
                validationDirectory,
                innerError); 
        }

        return restoreError;
    }

    ErrorCode EseLocalStore::PrepareRestore(wstring const & srcRestoreDirectory)
    {
        auto destRestoreDirectory = EseInstance::GetRestoreDirectoryName(directory_);

        auto error = Directory::Rename(srcRestoreDirectory, destRestoreDirectory, true); // overwrite

        if (error.IsSuccess())
        {
            WriteInfo(
                Constants::StoreSource,
                "{0} Successfully moved '{1}' to '{2}' for restore.",
                this->TraceId,
                srcRestoreDirectory,
                destRestoreDirectory); 
        }
        else
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} Failed to move from '{1}' to '{2}': {3}",
                this->TraceId,
                srcRestoreDirectory,
                destRestoreDirectory,
                error); 
        }

        return error;
    }

    ErrorCode EseLocalStore::MergeBackupChain(
        map<ULONG, wstring> const & backupChainOrderedDirList,
        __out wstring & mergedBackupDir)
    {
        ASSERT_IFNOT(
            !backupChainOrderedDirList.empty(),
            "{0} EseLocalStore::MergeBackupChain: backupChainOrderedDirList is empty.",
            this->TraceId);

        auto sIter = backupChainOrderedDirList.begin();
        
        // Set merge dir to full backup folder (first item is the full backup).
        mergedBackupDir = Path::Combine(sIter->second, FullBackupSubDirName); 
        
        for (auto iter = ++sIter; iter != backupChainOrderedDirList.end(); iter++)
        {
            auto error = Directory::Copy(iter->second, mergedBackupDir, true);
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        return ErrorCodeValue::Success;
    }

    bool EseLocalStore::IsLogTruncationRequired()
    {
        return settings_->IsIncrementalBackupEnabled;
    }

	bool EseLocalStore::IsIncrementalBackupEnabled()
	{
		return settings_->IsIncrementalBackupEnabled;
	}

    ErrorCode EseLocalStore::StartDefragmentation()
    {
        return defragmenter_->StartDefragmentation();
    }

    ErrorCode EseLocalStore::StopDefragmentation()
    {
        return defragmenter_->StopDefragmentation();
    }

    ErrorCode EseLocalStore::ForceMaxCacheSize()
    {
        return instance_->ForceMaxCacheSize();
    }

    ErrorCode EseLocalStore::PrefetchDatabaseTypes(std::vector<std::wstring> const & types)
    {
        WriteInfo(
            Constants::StoreSource,
            "{0} prefetching database types: {1}",
            this->TraceId,
            types);

        auto error =  this->ForceMaxCacheSize();

        if (!error.IsSuccess())
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} ForceMaxCacheSize failed: error={1}",
                this->TraceId,
                error);
            return error;
        }

        settings_->OptimizeTableScans = true;

        TransactionSPtr tx;
        error = this->CreateTransaction(tx); 
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                Constants::StoreSource,
                "{0} CreateTransaction failed: error={1}",
                this->TraceId,
                error);
            return error;
        }

        int count = 0;
        size_t totalBytes = 0;
        size_t charCount = 0;

        Stopwatch stopwatch;

        stopwatch.Start();

        for (auto const & type : types)
        {
            EnumerationSPtr en;
            error = this->CreateEnumerationByTypeAndKey(tx, type, L"", en);
            if (!error.IsSuccess()) 
            { 
                WriteWarning(
                    Constants::StoreSource,
                    "{0} CreateEnumerationByTypeAndKey failed: type={1} error={2}",
                    this->TraceId,
                    type,
                    error);
                return error;
            }

            while ((error = en->MoveNext()).IsSuccess())
            {
                wstring key;
                error = en->CurrentKey(key);
                if (!error.IsSuccess()) 
                { 
                    WriteWarning(
                        Constants::StoreSource,
                        "{0} CurrentKey failed: type={1} error={2}",
                        this->TraceId,
                        type,
                        error);
                    return error;
                }

                vector<byte> value;
                error = en->CurrentValue(value);
                if (!error.IsSuccess()) 
                {
                    WriteWarning(
                        Constants::StoreSource,
                        "{0} CurrentValue failed: type={1} key={2} error={3}",
                        this->TraceId,
                        type,
                        key,
                        error);
                    return error;
                }

                ++count;
                totalBytes += value.size();
                charCount += type.size() + key.size();

            } // move next

            if (error.IsError(ErrorCodeValue::EnumerationCompleted))
            {
                error = ErrorCodeValue::Success;
            }

            if (!error.IsSuccess())
            {
                WriteWarning(
                    Constants::StoreSource,
                    "{0} MoveNext failed: type={1} error={2}",
                    this->TraceId,
                    type,
                    error);

                return error;
            }

        } // for types

        stopwatch.Stop();

        auto throughput = stopwatch.Elapsed.TotalMilliseconds() > 0
            ? (static_cast<double>(count) / stopwatch.Elapsed.TotalMilliseconds()) * 1000
            : 0;

        WriteInfo(
            Constants::StoreSource,
            "{0} prefetch complete: count={1} value-bytes={2} key-bytes={3} elapsed={4} rows/sec={5}\n",
            this->TraceId,
            count,
            totalBytes,
            charCount * sizeof(wchar_t),
            stopwatch.Elapsed,
            throughput);

        return error;
    }

    ErrorCode EseLocalStore::ResetOptimizeTableScans()
    {
        settings_->OptimizeTableScans = false;

        auto error =  instance_->ResetCacheSize();

        if (!error.IsSuccess())
        {
            WriteWarning(
                Constants::StoreSource,
                "{0} ResetCacheSize failed: error={1}",
                this->TraceId,
                error);
        }

        return error;
    }

    ErrorCode EseLocalStore::JetToErrorCode(JET_ERR jetError)
    {
        if (instance_)
        {
            return instance_->JetToErrorCode(jetError);
        }
        else
        {
            return EseError::GetErrorCode(jetError);
        }
    }

    bool EseLocalStore::ShouldAutoCompact(int64 fileSize)
    {
        return EseInstance::ShouldAutoCompact(fileSize, settings_);
    }

    std::wstring EseLocalStore::GetValidationDirectoryName(std::wstring const & eseDirectory)
    {
        return wformatString("{0}_{1}", eseDirectory, *ValidationDirectorySuffix);
    }    
 
    /// <summary>
    /// A simple mapping function that takes in a backup option defined in ILocalStore.h
    /// and returns a JET_BitBackup option defined in esent.h
    /// </summary>
    /// <param name="backupOption">The backup options available in StoreBackupOption.h.</param>
    /// <param name="jetBackupOptions">The jet backup option that is populated by this method. Use this value only if this method returns ErrorCode::Success().</param>
    /// <returns>
    /// ErrorCode::Success() if the mapping was successful. An invalid argument error code value otherwise.
    /// </returns>
    ErrorCode EseLocalStore::GetJetBackupOptions(__in StoreBackupOption::Enum backupOption, __out LONG & jetBackupOptions)
    {        
        switch (backupOption)
        {
        case StoreBackupOption::Full:
        case StoreBackupOption::TruncateLogsOnly:
            jetBackupOptions = JET_bitBackupAtomic;
            return ErrorCode::Success();

        case StoreBackupOption::Incremental:
            jetBackupOptions = JET_bitBackupIncremental;
            return ErrorCode::Success();

        default:
            WriteWarning(
                Constants::StoreSource,
                "{0} Invalid argument '{1}' for backup options",
                this->TraceId,
                backupOption);

            jetBackupOptions = JET_errInvalidParameter;
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }
    }
}

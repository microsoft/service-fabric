// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "Store/test/Common.Test.h"

using namespace Common;
using namespace std;
using namespace TestHooks;

namespace Store
{

#define VERIFY( condition, fmt, ...) \
{ \
    auto tmp = wformatString(fmt, __VA_ARGS__); \
    if (!(condition)) \
    { \
        Trace.WriteError( \
            ReplicatedStoreTest::TestSource, \
            "VERIFY({0}) failed", \
            tmp); \
    } \
    VERIFY_IS_TRUE( condition, tmp.c_str() ); \
} \

#define VERIFY_IN_THREAD( condition, fmt, ...) \
{ \
    VERIFY( condition, fmt, __VA_ARGS__ ); \
} \

    typedef IStoreBase::TransactionSPtr TransactionSPtr;
    typedef IStoreBase::EnumerationSPtr EnumerationSPtr;
    static int const WaitOneTimeoutInSeconds(20);

    class ReplicatedStoreTest
    {
    public:
        static StringLiteral const TestSource;
        static wstring const DatabasePrimaryDirectory;
        static wstring const DatabaseSecondaryDirectory;
        static wstring const DatabaseFilename;
        static std::set<wstring> KnownDataTypes;
        static RwLock KnownDataTypesLock;

        ReplicatedStoreTest() { BOOST_REQUIRE(MethodSetup()); }
        TEST_METHOD_SETUP(MethodSetup);
        ~ReplicatedStoreTest() { BOOST_REQUIRE(MethodCleanup()); }
        TEST_METHOD_CLEANUP(MethodCleanup);

        void TombstoneParsingHelper(__in ReplicatedStore &, wstring const &, wstring const &);
        void DeleteDirectory(wstring const &);
    };

    StringLiteral const ReplicatedStoreTest::TestSource("ReplicatedStoreTest");
    wstring const ReplicatedStoreTest::DatabasePrimaryDirectory(L"ReplicatedStorePrimaryFiles");
    wstring const ReplicatedStoreTest::DatabaseSecondaryDirectory(L"ReplicatedStoreSecondaryFiles");
    wstring const ReplicatedStoreTest::DatabaseFilename(L"ReplicatedStoreData.edb");
    std::set<wstring> ReplicatedStoreTest::KnownDataTypes;
    RwLock ReplicatedStoreTest::KnownDataTypesLock;

    // **************
    // Helper classes
    // **************

    class ComMockStatefulServicePartition :
        public IFabricStatefulServicePartition,
        public ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComMockStatefulServicePartition,
            IID_IFabricStatefulServicePartition,
            IFabricStatefulServicePartition)

    public:
        ComMockStatefulServicePartition(
            ::FABRIC_PARTITION_ID partitionId)
            : partitionInfo_()
            , isAccessDenied_(false)
        {
            partitionInfo_.Kind = FABRIC_SERVICE_PARTITION_KIND_INT64_RANGE;
            int64PartitionInfo_.Id = partitionId;
            int64PartitionInfo_.LowKey = 0;
            int64PartitionInfo_.HighKey = 0;
            int64PartitionInfo_.Reserved = NULL;
            partitionInfo_.Value = reinterpret_cast<void*>(&int64PartitionInfo_);
            faultCount_.store(0);
        }

        void SetAccessDenied()
        {
            isAccessDenied_ = true;
        }

        HRESULT STDMETHODCALLTYPE GetPartitionInfo(__out const ::FABRIC_SERVICE_PARTITION_INFORMATION ** result)
        {
            if (result == NULL) { return E_POINTER; }
            (*result) = &partitionInfo_;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetReadStatus(__out ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS * result)
        {
            if (result == NULL) { return E_POINTER; }

            (*result) = isAccessDenied_ ? 
                ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING : 
                ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetWriteStatus(__out ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS * result)
        {
            if (result == NULL) { return E_POINTER; }

            (*result) = isAccessDenied_ ? 
                ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING : 
                ::FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED;

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE CreateReplicator(__in IFabricStateProvider *, __in_opt const FABRIC_REPLICATOR_SETTINGS *, __out IFabricReplicator **, __out IFabricStateReplicator **)
        {
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE ReportLoad(__in ULONG metricCount, __in const ::FABRIC_LOAD_METRIC *metrics)
        {
            UNREFERENCED_PARAMETER(metricCount);
            UNREFERENCED_PARAMETER(metrics);

            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE ReportFault(FABRIC_FAULT_TYPE faultType)
        {
            UNREFERENCED_PARAMETER(faultType);

            ++faultCount_;

            return E_NOTIMPL;
        }

        LONG GetFaultCount()
        {
            return faultCount_.load();
        }

    private:
        ::FABRIC_SERVICE_PARTITION_INFORMATION partitionInfo_;
        ::FABRIC_INT64_RANGE_PARTITION_INFORMATION int64PartitionInfo_;
        Common::atomic_long faultCount_;
        bool isAccessDenied_;
    };

    class MockReplicationOperation :
        public IFabricOperation,
        public ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComMockReplicationOperation,
            IID_IFabricOperation,
            IFabricOperation)

    public:
        MockReplicationOperation(
            ::FABRIC_OPERATION_TYPE type,
            ::FABRIC_SEQUENCE_NUMBER sequenceNumber,
            ComPointer<IFabricOperationData> && operationData)
            : metadata_() 
            , operationData_(std::move(operationData))
        {
            metadata_.Type = type;
            metadata_.SequenceNumber = sequenceNumber;
        }

        const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE get_Metadata(void)
        {
            return &metadata_;
        }

        HRESULT STDMETHODCALLTYPE GetData(__out ULONG * len, __out FABRIC_OPERATION_DATA_BUFFER const ** result)
        {
            if (len == NULL || result == NULL) { return E_POINTER; }
            if (operationData_)
            {
                return operationData_->GetData(len, result);
            }

            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE Acknowledge()
        {
            return S_OK;
        }

    private:
        ::FABRIC_OPERATION_METADATA metadata_;
        ComPointer<IFabricOperationData> operationData_;
    };

    class QueuedReplicationOperation
    {
    public:
        QueuedReplicationOperation(
            ::FABRIC_OPERATION_TYPE type,
            ::FABRIC_SEQUENCE_NUMBER sequenceNumber,
            ComPointer<IFabricOperationData> && operationData)
            : replicationOperationCPtr_(make_com<MockReplicationOperation, IFabricOperation>(type, sequenceNumber, std::move(operationData))) 
        {
        }

        __declspec(property(get=get_Operation)) ComPointer<IFabricOperation> const & Operation;
        ComPointer<IFabricOperation> const & get_Operation() const { return replicationOperationCPtr_; }

    private:
        ComPointer<IFabricOperation> replicationOperationCPtr_;
    };

    // ComMockReplicator mocks up queueing of replication and copy operations
    // between a primary and a secondary
    //
    class ComMockReplicator : 
        public IFabricReplicator,
        public IFabricStateReplicator,
        public IFabricInternalStateReplicator,
        public ComUnknownBase
    {
        BEGIN_COM_INTERFACE_LIST(MockReplicator)
            COM_INTERFACE_ITEM(IID_IUnknown,IFabricStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricReplicator,IFabricReplicator)
            COM_INTERFACE_ITEM(IID_IFabricStateReplicator,IFabricStateReplicator)
            COM_INTERFACE_ITEM(IID_IFabricInternalStateReplicator,IFabricInternalStateReplicator)
        END_COM_INTERFACE_LIST()

    public:
        typedef ReaderQueue<QueuedReplicationOperation> ReplicationQueue;
        typedef shared_ptr<ReplicationQueue> ReplicationQueueSPtr;

        explicit ComMockReplicator(
            bool replicateSynchronously = true,
            DWORD delayReplicateInMilliseconds = 0)
            : replicateSynchronously_(replicateSynchronously)
            , delayReplicateInMilliseconds_(delayReplicateInMilliseconds)
            , copyQueueSPtr_(ReplicationQueue::Create())
            , replicationQueueSPtr_(ReplicationQueue::Create())
            , operationLSN_(0)
            , notificationLSN_(0)
            , verifyReplicationSuccess_(true)
        {
        }

        ~ComMockReplicator()
        {
            ASSERT_IF(copyQueueSPtr_->ItemCount > 0, "Replicator not empty: item count {0}", copyQueueSPtr_->ItemCount);
            ASSERT_IF(copyQueueSPtr_->PendingReaderCount > 0, "Replicator not empty: reader count {0}", copyQueueSPtr_->PendingReaderCount);
            ASSERT_IF(replicationQueueSPtr_->ItemCount > 0, "Replicator not empty: item count {0}", replicationQueueSPtr_->ItemCount);
            ASSERT_IF(replicationQueueSPtr_->PendingReaderCount > 0, "Replicator not empty: reader count {0}", replicationQueueSPtr_->PendingReaderCount);
        }

        void SetVerifyReplicationSuccess(bool);

        void ResetQueue(); 
        void ResetOperationLSNs(LONG lsn = 0); 
        void SetNotificationLsn(LONG lsn);
        void Close() { replicationQueueSPtr_->Close(); copyQueueSPtr_->Close(); }
        void Drain();
        void DrainCopy();
        void DrainAndClose();

        void QueueLastCopyOperation();
        void QueueCopyOperations(
            __in ReplicatedStore & primaryStore,
            __in ReplicatedStore & secondaryStore,
            ::FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
            ErrorCodeValue::Enum expectedError = ErrorCodeValue::Success,
            bool queueLastCopyOperation = true,
            int maxCount = numeric_limits<int>::max());

        __declspec(property(get=get_ReplicationItemCount)) size_t ReplicationItemCount;
        __declspec(property(get=get_CopyItemCount)) size_t CopyItemCount;
        __declspec(property(get=get_LastOperationLSN)) ::FABRIC_SEQUENCE_NUMBER LastOperationLSN;
        __declspec(property(get=get_LastNotificationLSN)) ::FABRIC_SEQUENCE_NUMBER LastNotificationLSN;

        size_t get_CopyItemCount() const { return copyQueueSPtr_->ItemCount; }
        size_t get_ReplicationItemCount() const { return replicationQueueSPtr_->ItemCount; }
        ::FABRIC_SEQUENCE_NUMBER get_LastOperationLSN() const { return operationLSN_.load(); }
        ::FABRIC_SEQUENCE_NUMBER get_LastNotificationLSN() const { return notificationLSN_.load(); }

    public:

        //
        // IFabricStateReplicator
        //

        HRESULT STDMETHODCALLTYPE BeginReplicate(
            __in IFabricOperationData *,
            __in IFabricAsyncOperationCallback *,
            __out ::FABRIC_SEQUENCE_NUMBER *,
            __out IFabricAsyncOperationContext **);

        HRESULT STDMETHODCALLTYPE EndReplicate(
            __in IFabricAsyncOperationContext *,
            __out ::FABRIC_SEQUENCE_NUMBER *);

        HRESULT STDMETHODCALLTYPE GetReplicationStream(
            __out IFabricOperationStream **);

        HRESULT STDMETHODCALLTYPE GetCopyStream(
            __out IFabricOperationStream **);

        HRESULT STDMETHODCALLTYPE UpdateReplicatorSettings(
            __in FABRIC_REPLICATOR_SETTINGS const * )
        {
            return E_NOTIMPL;
        }

    public:
        
        //
        // IFabricInternalStateReplicator
        //

        HRESULT STDMETHODCALLTYPE ReserveSequenceNumber(
            /* [in] */ BOOLEAN alwaysReserveWhenPrimary,
            /* [out, retval] */ FABRIC_SEQUENCE_NUMBER * sequenceNumber)
        {
            UNREFERENCED_PARAMETER(alwaysReserveWhenPrimary);
            UNREFERENCED_PARAMETER(sequenceNumber);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE BeginReplicateBatch(
            /* [in] */ IFabricBatchOperationData * operationData,
            /* [in] */ IFabricAsyncOperationCallback * callback,
            /* [out, retval] */ IFabricAsyncOperationContext ** context)
        {
            UNREFERENCED_PARAMETER(operationData);
            UNREFERENCED_PARAMETER(callback);
            UNREFERENCED_PARAMETER(context);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE EndReplicateBatch(
            /* [in] */ IFabricAsyncOperationContext * context)
        {
            UNREFERENCED_PARAMETER(context);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetBatchReplicationStream(
            /* [out, retval] */ IFabricBatchOperationStream **batchStream)
        {
            UNREFERENCED_PARAMETER(batchStream);
            return E_NOTIMPL;
        }

        HRESULT STDMETHODCALLTYPE GetReplicationQueueCounters(
            /* [out] */ FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS * counters)
        {
            counters->operationCount = 0;
            counters->queueSizeBytes = 0;
            return S_OK;
        }

    public:

        //
        // IFabricReplicator
        //

        HRESULT BeginOpen(
            /* [in] */ IFabricAsyncOperationCallback *,
            /* [out, retval] */ IFabricAsyncOperationContext **)
        {
            return E_NOTIMPL;
        }
        HRESULT EndOpen(
            /* [in] */ IFabricAsyncOperationContext *,
            /* [out, retval] */ IFabricStringResult **)
        {
            return E_NOTIMPL;
        }

        HRESULT BeginChangeRole(
            /* [in] */ FABRIC_EPOCH const *,
            /* [in] */ FABRIC_REPLICA_ROLE,
            /* [in] */ IFabricAsyncOperationCallback *,
            /* [out, retval] */ IFabricAsyncOperationContext **)
        {
            return E_NOTIMPL;
        }
        HRESULT EndChangeRole(
            /* [in] */ IFabricAsyncOperationContext *)
        {
            return E_NOTIMPL;
        }

        HRESULT BeginUpdateEpoch(
            /* [in] */ FABRIC_EPOCH const *,
            /* [in] */ IFabricAsyncOperationCallback *,
            /* [out, retval] */ IFabricAsyncOperationContext **)
        {
            return E_NOTIMPL;
        }
        HRESULT EndUpdateEpoch(
            /* [in] */ IFabricAsyncOperationContext *)
        {
            return E_NOTIMPL;
        }

        HRESULT BeginClose(
            /* [in] */ IFabricAsyncOperationCallback * ,
            /* [out, retval] */ IFabricAsyncOperationContext **)
        {
            return E_NOTIMPL;
        }
        HRESULT EndClose(
            /* [in] */ IFabricAsyncOperationContext *)
        {
            return E_NOTIMPL;
        }

        void Abort()
        {
        }

        HRESULT GetCurrentProgress(
            /* [out] */ FABRIC_SEQUENCE_NUMBER *)
        {
            return E_NOTIMPL;
        }

        HRESULT GetCatchUpCapability(
            /* [out] */ FABRIC_SEQUENCE_NUMBER *)
        {
            return E_NOTIMPL;
        }

    private:
        // Wrapper class for IFabricAsyncOperationCallback::Begin/EndGetNext()
        class ComGetNextAsyncOperation : public ComProxyAsyncOperation
        {
        public:
            ComGetNextAsyncOperation(
                ComPointer<IFabricOperationDataStream> const & enumerator,
                AsyncCallback const & callback,
                AsyncOperationSPtr const & root)
                : ComProxyAsyncOperation(callback, root)
                , enumerator_(enumerator)
            {
            }

            static ErrorCode End(AsyncOperationSPtr const &, __out ComPointer<IFabricOperationData> &);

        protected:
            HRESULT BeginComAsyncOperation(__in IFabricAsyncOperationCallback *, __out IFabricAsyncOperationContext **);
            HRESULT EndComAsyncOperation(__in IFabricAsyncOperationContext *);

        private:
            ComPointer<IFabricOperationDataStream> enumerator_;
            ComPointer<IFabricOperationData> copyOperation_;
        };

        // IFabricAsyncOperationContext for Begin/EndReplicate() and Begin/EndGetOperation()
        class ComReplicatedOperationContext :
            public IFabricAsyncOperationContext,
            public ComUnknownBase
        {
            COM_INTERFACE_LIST1(
                ComReplicatedOperationContext,
                IID_IFabricAsyncOperationContext,
                IFabricAsyncOperationContext)
        public:
            ComReplicatedOperationContext(
                ReplicationQueueSPtr const & queueSPtr,
                ComPointer<IFabricAsyncOperationCallback> const & callback) 
                : queueSPtr_(queueSPtr) 
                , callbackCPtr_(callback)
                , completedSynchronously_(false)
            { 
            }

            ~ComReplicatedOperationContext()
            {
            }

            BOOLEAN STDMETHODCALLTYPE IsCompleted() { Assert::CodingError("IsCompleted() not implemented"); }
            BOOLEAN STDMETHODCALLTYPE CompletedSynchronously();
            HRESULT STDMETHODCALLTYPE get_Callback(IFabricAsyncOperationCallback **) { Assert::CodingError("CallbackState() not implemented"); }
            HRESULT STDMETHODCALLTYPE Cancel() 
            { 
                Trace.WriteWarning(ReplicatedStoreTest::TestSource, "ComReplicatedOperationContext::Cancel() not implemented");
                return S_OK;
            }

            void EnqueueAndDispatch(
                unique_ptr<QueuedReplicationOperation> &&, 
                bool synchronous, 
                DWORD delayReplicateInMilliseconds, 
                bool verifySuccess);
            void BeginDequeue();
            void EndDequeue(__out ComPointer<IFabricOperation> &);

        private:
            void OnDequeueComplete(AsyncOperationSPtr const &);
            void Complete(bool synchronous, DWORD delayReplicateInMilliseconds);
            void CompleteNow();

            ReplicationQueueSPtr queueSPtr_;
            ComPointer<IFabricAsyncOperationCallback> callbackCPtr_;

            bool completedSynchronously_;
            shared_ptr<AsyncOperation> innerAsyncOperation_;
            unique_ptr<QueuedReplicationOperation> queuedOperationUPtr_;
        };

        class ComMockOperationStream :
            public IFabricOperationStream, 
            private Common::ComUnknownBase
        {
            DENY_COPY(ComMockOperationStream)

                COM_INTERFACE_LIST1(
                ComMockOperationStream,
                IID_IFabricOperationStream,
                IFabricOperationStream)
        public:
            explicit ComMockOperationStream(ReplicationQueueSPtr const & queueSPtr) 
                : ComUnknownBase(), queueSPtr_(queueSPtr)
            {
            }
            virtual ~ComMockOperationStream() {}

            virtual HRESULT STDMETHODCALLTYPE BeginGetOperation(
                /* [in] */ __in IFabricAsyncOperationCallback * callback,
                /* [out, retval] */ __out IFabricAsyncOperationContext ** context);

            virtual HRESULT STDMETHODCALLTYPE EndGetOperation(
                /* [in] */ __in IFabricAsyncOperationContext * context,
                /* [out, retval] */ __out IFabricOperation ** operation);

        private:
            ReplicationQueueSPtr queueSPtr_;
        };

        bool replicateSynchronously_;
        DWORD delayReplicateInMilliseconds_;
        ReplicationQueueSPtr copyQueueSPtr_;
        ReplicationQueueSPtr replicationQueueSPtr_;
        Common::atomic_long operationLSN_;
        Common::atomic_long notificationLSN_;
        bool verifyReplicationSuccess_;
    };

    // Structural component root for a ReplicatedStore instance
    //
    class ReplicaBase 
        : public ComponentRoot
        , public Api::ISecondaryEventHandler
        , public PartitionedReplicaTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        static wstring const DefaultTypePrefix;
        static wstring const DefaultKeyPrefix;

        class DataItem
        {
        public:
            DataItem(
                wstring type,
                wstring key,
                _int64 operationLSN,
                vector<byte> const & bytes,
                FILETIME lastModifiedOnPrimaryUtc)
                : type_(type)
                , key_(key)
                , operationLSN_(operationLSN)
                , bytes_(bytes)
                , lastModifiedOnPrimaryUtc_(lastModifiedOnPrimaryUtc)
            { 
            }

            __declspec(property(get=get_Type)) wstring const & Type;
            __declspec(property(get=get_Key)) wstring const & Key;
            __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;
            __declspec(property(get=get_Bytes)) vector<byte> const & Bytes;
            __declspec(property(get = get_LastModifiedOnPrimaryUtc)) FILETIME const & LastModifiedOnPrimaryUtc;
            __declspec(property(get = get_LastModifiedOnPrimaryInt64)) _int64 LastModifiedOnPrimaryInt64;

            wstring const & get_Type() const { return type_; }
            wstring const & get_Key() const { return key_; }
            ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }
            vector<byte> const & get_Bytes() const { return bytes_; }
            FILETIME const & get_LastModifiedOnPrimaryUtc() const { return lastModifiedOnPrimaryUtc_; }
            
            _int64 get_LastModifiedOnPrimaryInt64() const
            {
                ULARGE_INTEGER convert;
                convert.LowPart = lastModifiedOnPrimaryUtc_.dwLowDateTime;
                convert.HighPart = lastModifiedOnPrimaryUtc_.dwHighDateTime;

                return static_cast<_int64>(convert.QuadPart);
            }

        private:
            wstring type_;
            wstring key_;
            ::FABRIC_SEQUENCE_NUMBER operationLSN_;
            vector<byte> bytes_;
            FILETIME lastModifiedOnPrimaryUtc_;
        };

        __declspec(property(get=get_Store)) ReplicatedStore & Store;
        ReplicatedStore & get_Store() { return *storeUPtr_; }
        void SetStore(unique_ptr<ReplicatedStore> && store) { storeUPtr_.swap(store); }
        
        void UpdateSettings(ReplicatedStoreSettings const & settings)
        {
            replicatedStoreSettings_ = settings;
        }

        void Open(
            ::FABRIC_REPLICA_ROLE role, 
            ErrorCodeValue::Enum const & expectedError = ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE mode = FABRIC_REPLICA_OPEN_MODE_NEW,
            bool enableIncrementalBackup = false);

        void ChangeRole(::FABRIC_REPLICA_ROLE role);
        void UpdateEpoch(LONGLONG configurationNumber, LONGLONG datalossNumber, ::FABRIC_SEQUENCE_NUMBER);
        void Close(ErrorCodeValue::Enum allowedError = ErrorCodeValue::Success);
        void Abort();
        void AsyncAbort();

        void SetAccessDenied();

        vector<DataItem> ReadUserData(
            _int64 startOperationLSN = 0, 
            _int64 endOperationLSN = numeric_limits<_int64>::max());
        vector<DataItem> GetNotificationData(
            _int64 startOperationLSN, 
            _int64 endOperationLSN);
        vector<DataItem> GetNotificationDataToVerify(vector<DataItem> const &);
        void SortData(vector<DataItem> &);

        _int64 ReadProgress();
        vector<ProgressVectorEntry> ReadProgressVector(bool includeCurrentEpoch = true);

        void VerifyData(
            vector<DataItem> const & dataToVerify, 
            _int64 startOperationLSN = 0, 
            _int64 endOperationLSN = numeric_limits<_int64>::max());
        void VerifyEqualData(
            vector<DataItem> const & leftData, 
            vector<DataItem> const & rightData,
            _int64 startOperationLSN,
            _int64 endOperationLSN); 
        void VerifyProgress(_int64 expectedProgress);
        void VerifyProgressVector(vector<ProgressVectorEntry> const &, bool includeCurrentEpoch = true);

        long GetFaultCount();
        void VerifyFaultCount();
        void VerifyFaultCount(long expected);
        void VerifyCopyCounts(uint64 fullCopyCount, uint64 partialCopyCount);

        FABRIC_SEQUENCE_NUMBER GetTombstoneLowWatermark();
        void VerifyTombstoneLowWatermark(FABRIC_SEQUENCE_NUMBER toVerify);
        void VerifyTombstones1(vector<DataItem> const &, size_t expectedSize);
        void VerifyTombstones2(vector<DataItem> const &, size_t expectedSize);
        void VerifyTombstones2(vector<DataItem> const &, size_t expectedSize, size_t expectedEstimatedSize);
        void VerifyTombstones(vector<DataItem> const &, size_t expectedSize, size_t expectedEstimatedSize, bool expectVersion1);
        void VerifyTombstoneOrder(vector<DataItem> const &, FABRIC_SEQUENCE_NUMBER startingLsn, size_t expectedSize);
        vector<DataItem> WaitForDataCount(size_t expectedCount);

        void InsertData(
            int typeCount, 
            int keyCount, 
            int bytesCount, 
            int targetModulo = 1, 
            wstring const & = DefaultTypePrefix, 
            wstring const & = DefaultKeyPrefix,
            ErrorCodeValue::Enum retryableError = ErrorCodeValue::Success);
        void UpdateData(
            int typeCount, 
            int keyCount, 
            int bytesCount, 
            int targetModulo, 
            wstring const & = DefaultTypePrefix, 
            wstring const & = DefaultKeyPrefix);
        void DeleteData(
            int typeCount, 
            int keyCount, 
            int targetModulo, 
            wstring const & = DefaultTypePrefix, 
            wstring const & = DefaultKeyPrefix,
            ErrorCodeValue::Enum retryableError = ErrorCodeValue::Success);
        void WriteData(
            int typeCount, 
            int keyCount, 
            int bytesCount, 
            int targetModulo,
            wstring const &,
            wstring const &,
            ReplicationOperationType::Enum operation,
            ErrorCodeValue::Enum retryableError = ErrorCodeValue::Success);
        void InnerWriteData(
            __in TransactionSPtr &,
            int typeCount, 
            int keyCount, 
            int bytesCount, 
            int targetModulo,
            wstring const &,
            wstring const &,
            ReplicationOperationType::Enum operation,
            ErrorCodeValue::Enum retryableError = ErrorCodeValue::Success);

        void CreateDatabaseDirectoryName(__out std::wstring & dbDirectory);

        void SetCopyNotificationComplete();
        void SetCopyNotificationDelay(TimeSpan const);
        void ResetReplicationNotificationComplete();

    protected:
        ReplicaBase(
            wstring databaseDirectory,
            wstring databaseFilename,
            ::FABRIC_PARTITION_ID partitionId,
            ::FABRIC_REPLICA_ID replicaId,
            __in ComPointer<ComMockReplicator> const & replicator)
            : PartitionedReplicaTraceComponent(Guid(partitionId), replicaId)
            , databaseDirectory_(databaseDirectory)
            , databaseFilename_(databaseFilename)
            , partitionCPtr_(make_com<ComMockStatefulServicePartition>(partitionId))
            , replicaId_(replicaId)
            , replicatorCPtr_(replicator)
            , notificationItems_()
            , copyNotificationLsn_(0)
            , copyNotificationProcessed_(false)
            , replNotificationProcessed_(false)
            , copyNotificationDelay_(TimeSpan::Zero)
            , replicatedStoreSettings_()
        {
        }

    private:
        Common::ErrorCode OnCopyComplete(Api::IStoreEnumeratorPtr const &);
        Common::ErrorCode OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const &);

        wstring databaseDirectory_;
        wstring databaseFilename_;
        ComPointer<ComMockStatefulServicePartition> partitionCPtr_;
        ::FABRIC_REPLICA_ID replicaId_;
        ComPointer<ComMockReplicator> const & replicatorCPtr_;
        unique_ptr<ReplicatedStore> storeUPtr_;

        std::vector<DataItem> notificationItems_;
        ::FABRIC_SEQUENCE_NUMBER copyNotificationLsn_;
        ManualResetEvent copyNotificationProcessed_;
        ManualResetEvent replNotificationProcessed_;
        TimeSpan copyNotificationDelay_;

        ReplicatedStoreSettings replicatedStoreSettings_;
    };
    typedef shared_ptr<ReplicaBase> ReplicaBaseSPtr;

    wstring const ReplicaBase::DefaultTypePrefix = L"Type_";
    wstring const ReplicaBase::DefaultKeyPrefix = L"Key_";

    class PrimaryReplica : public ReplicaBase
    {
    public:
        static shared_ptr<PrimaryReplica> Create(__in ComPointer<ComMockReplicator> const & replicator)
        {
            return shared_ptr<PrimaryReplica>(new PrimaryReplica(replicator));
        }

    private:
        PrimaryReplica(__in ComPointer<ComMockReplicator> const & replicator) : ReplicaBase(
            ReplicatedStoreTest::DatabasePrimaryDirectory,
            ReplicatedStoreTest::DatabaseFilename,
            Guid(L"00000000-0000-0000-0000-000000000000").AsGUID(),
            1,
            replicator)
        {
        }
    };
    typedef shared_ptr<ReplicaBase> PrimaryReplicaSPtr;

    class SecondaryReplica : public ReplicaBase
    {
    public:
        static shared_ptr<SecondaryReplica> Create(__in ComPointer<ComMockReplicator> const & replicator)
        {
            return shared_ptr<SecondaryReplica>(new SecondaryReplica(replicator));
        }

    private:
        SecondaryReplica(__in ComPointer<ComMockReplicator> const & replicator) : ReplicaBase(
            ReplicatedStoreTest::DatabaseSecondaryDirectory,
            ReplicatedStoreTest::DatabaseFilename,
            Guid(L"00000000-0000-0000-0000-000000000000").AsGUID(),
            2,
            replicator)
        {
        }
    };
    typedef shared_ptr<ReplicaBase> SecondaryReplicaSPtr;

    class PostBackupAsyncOperation : public AsyncOperation
    {
        DENY_COPY(PostBackupAsyncOperation);

    public:
        PostBackupAsyncOperation(
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent)
            : AsyncOperation(callback, parent)
        {
        }

    protected:
        void OnStart(AsyncOperationSPtr const & proxySPtr)
        {
            this->TryComplete(proxySPtr, ErrorCode(ErrorCodeValue::Success));
        }
    };

    class PostBackupHandler 
        : public Api::IStorePostBackupHandler
        , public Common::ComponentRoot
    {

    public:
        virtual Common::AsyncOperationSPtr BeginPostBackup(
            __in FABRIC_STORE_BACKUP_INFO const & info,
            __in AsyncCallback const & callback,
            __in AsyncOperationSPtr const & parent) override
        {
            UNREFERENCED_PARAMETER(info);
            return AsyncOperation::CreateAndStart<PostBackupAsyncOperation>(callback, parent);
        }

        virtual Common::ErrorCode EndPostBackup(
            __in Common::AsyncOperationSPtr const & operation,
            __out bool & status) override
        {
            UNREFERENCED_PARAMETER(operation);
            status = true;
            return ErrorCodeValue::Success;
        }
    };
    typedef shared_ptr<PostBackupHandler> PostBackupHandlerSPtr;

    class BackupRestoreHelper
    {
    public:
        BackupRestoreHelper();
        ~BackupRestoreHelper();

        void InserDataAndTakeBackups(
            ReplicaBaseSPtr replica, 
            std::vector<ReplicaBase::DataItem> & origData1,
            std::vector<ReplicaBase::DataItem> & origData2);

        void VerifyEmptyDirectoryValidation(ReplicaBaseSPtr replica);
        void VerifyInvalidBackupChainValidation1(ReplicaBaseSPtr replica);
        void VerifyDuplicateFullBackupValidation1(ReplicaBaseSPtr replica);
        void VerifyDuplicateFullBackupValidation2(ReplicaBaseSPtr replica);
        void VerifyInvalidBackupChainValidation2(ReplicaBaseSPtr replica);
		void VerifyBackupWithMissingFile(ReplicaBaseSPtr replica);
		void VerifyBackupWithExtraFile(ReplicaBaseSPtr replica);

        void VerifyRestoreFromBackupChain(
            ReplicaBaseSPtr newReplica,
            std::vector<ReplicaBase::DataItem> const & origData1,
            std::vector<ReplicaBase::DataItem> const & origData2);
        
        ErrorCode TakeBackup(ReplicaBaseSPtr replica, wstring const & backupDir, StoreBackupOption::Enum backupOption);
        ErrorCode TakeBackup(ReplicaBaseSPtr replica, StoreBackupOption::Enum backupOption);
        ErrorCode DoRestore(ReplicaBaseSPtr replica, RestoreSettings restoreSettings);
        void InsertRandomData(ReplicaBaseSPtr replica);

    private:
        std::wstring backupBaseDir_;
        std::wstring backupDirFull_;
        std::wstring backupDirInc_;
        std::wstring restoreDir_;
        std::wstring restoreDirFull_;
        std::wstring restoreDirInc_;
        PostBackupHandlerSPtr backupHandler_;
        _int64 incBackupCount_;
    };

    // ********************* 
    // Test cases start here
    // ********************* 

    BOOST_FIXTURE_TEST_SUITE(ReplicatedStoreTestSuite, ReplicatedStoreTest)

    BOOST_AUTO_TEST_CASE(LifecycleOpenCloseTest)
    {
        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        
        VERIFY_IS_TRUE(primary->Store.State.Value == Common::FabricComponentState::Opened);

        primary->Close();

        VERIFY_IS_TRUE(primary->Store.State.Value == Common::FabricComponentState::Closed);
    }

    BOOST_AUTO_TEST_CASE(LifecycleOpenAbortTest)
    {
        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        
        VERIFY_IS_TRUE(primary->Store.State.Value == Common::FabricComponentState::Opened);

        primary->Abort();

        VERIFY_IS_TRUE(primary->Store.State.Value == Common::FabricComponentState::Aborted);
    }

    BOOST_AUTO_TEST_CASE(LifecycleOpenThrottleTest)
    {
        StoreConfig::GetConfig().OpenLocalStoreThrottle = 10;
        size_t targetCount = 100;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** LifecycleOpenThrottleTest(count={0} throttle={1})", targetCount, StoreConfig::GetConfig().OpenLocalStoreThrottle);

        vector<ReplicatedStoreUPtr> stores;
        stores.resize(targetCount);

        auto root = make_shared<ComponentRoot>();

        for (auto ix=0; ix<stores.size(); ++ix)
        {
            stores[ix] = EseReplicatedStore::Create(
                Guid::NewGuid(),
                ix, 
                make_com<ComMockReplicator, IFabricReplicator>(),
                ReplicatedStoreSettings(),
                EseLocalStoreSettings(L"throttle_test.edb", L"throttle_test"),
                Api::IStoreEventHandlerPtr(),
                Api::ISecondaryEventHandlerPtr(),
                *root);
        }

        ManualResetEvent event(false);
        atomic_uint64 pendingCount(stores.size());

        Stopwatch stopwatch;
        stopwatch.Start();

        for (auto & store : stores)
        {
            auto comPartition = make_com<ComMockStatefulServicePartition>(store->PartitionId.AsGUID());

            ComPointer<IFabricStatefulServicePartition> partition(comPartition.GetRawPointer(), IID_IFabricStatefulServicePartition);

            store->BeginOpen(
                FABRIC_REPLICA_OPEN_MODE_NEW,
                partition,
                [&store, &event, &pendingCount](AsyncOperationSPtr const & operation) 
                { 
                    ComPointer<IFabricReplicator> unused;
                    auto error = store->EndOpen(operation, unused);
                    VERIFY_IS_TRUE(error.IsSuccess());

                    if (--pendingCount <= 0)
                    {
                        event.Set();
                    }
                },
                root->CreateAsyncOperationRoot());
        }

        event.WaitOne();

        stopwatch.Stop();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Elapsed open = {0}", stopwatch.Elapsed);

        event.Reset();
        pendingCount.store(stores.size());
        stopwatch.Restart();

        for (auto & store : stores)
        {
            store->BeginClose(
                [&store, &event, &pendingCount](AsyncOperationSPtr const & operation) 
                { 
                    auto error = store->EndClose(operation);
                    VERIFY_IS_TRUE(error.IsSuccess());

                    if (--pendingCount <= 0)
                    {
                        event.Set();
                    }
                },
                root->CreateAsyncOperationRoot());
        }

        event.WaitOne();

        stopwatch.Stop();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Elapsed close = {0}", stopwatch.Elapsed);
    }

    BOOST_AUTO_TEST_CASE(BatchingCommitTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** BatchingCommitTest");

        char data[] = "AbCDefGHIjklmNOPqrStuvWxyZ";

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

#define N 10
        ManualResetEvent testEvent[N];

        for (int i=0; i<N; i++)
        {
            Threadpool::Post([&, i]()
            {
                Common::Random rand;
                ::Sleep(rand.Next(1, 5));

                std::wstring type;
                Common::StringWriter typeWriter(type);
                typeWriter.Write("type{0}", i+4);

                std::wstring key;
                Common::StringWriter keyWriter(key);
                keyWriter.Write("key{0}", i+4);

                TransactionSPtr txSPtr;
                ErrorCode error = primary->Store.CreateSimpleTransaction(txSPtr);
                VERIFY_IS_TRUE(error.IsSuccess());

                error = primary->Store.Insert(
                    txSPtr, 
                    type,
                    key,
                    &data[i], 
                    1);
                if (error.IsSuccess())
                {
                    error = txSPtr->Commit();
                }
                else
                {
                    error = primary->Store.Update(
                        txSPtr, 
                        type, 
                        key, 
                        0,
                        key,
                        &data[i], 
                        2);

                    if (error.IsSuccess())
                    {
                        error = txSPtr->Commit();
                    }
                    else
                    {
                        txSPtr->Rollback();
                    }
                }
                testEvent[i].Set();
            });
        }

        for (int i=0; i<N; i++)
        {
            testEvent[i].WaitOne();
        }

        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();
        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);

        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Replicate operations from primary to secondary.
    // Database contents should match afterwards.
    //
    BOOST_AUTO_TEST_CASE(ReplicationTest)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ReplicationTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY); 
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->VerifyProgress(1);

        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2); 
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5); 
        primary->VerifyProgress(3);

        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6); 

        // Drain replication queue to secondary
        //

        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation(); 

        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify); 
        secondary->VerifyProgress(6);

        // Ensure we are in the secondary role to cover closing from this state
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->Close();
        secondary->Close();
        primary->Close();
    }

#if !defined(PLATFORM_UNIX)

    // This test is intended to verify the validation logic for backup chain metadata that
    // is generated by replicated store during backup operation and validated during restore.
    // There are separate tests that tests the actual backup-restore functionality.
#if !defined(PLATFORM_UNIX)
    BOOST_AUTO_TEST_CASE(BackupChainRestoreTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** BackupChainRestoreTest ***");

        BackupRestoreHelper backupRestoreHelper;
        std::vector<ReplicaBase::DataItem> origData1;
        std::vector<ReplicaBase::DataItem> origData2;

        {
            auto replicator = make_com<ComMockReplicator>();
            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            primary->UpdateEpoch(1, 0, 0);

            backupRestoreHelper.InserDataAndTakeBackups(primary, origData1, origData2);
            backupRestoreHelper.VerifyEmptyDirectoryValidation(primary);
            backupRestoreHelper.VerifyInvalidBackupChainValidation1(primary);
            backupRestoreHelper.VerifyDuplicateFullBackupValidation1(primary);
            backupRestoreHelper.VerifyDuplicateFullBackupValidation2(primary);
            backupRestoreHelper.VerifyInvalidBackupChainValidation2(primary);
			backupRestoreHelper.VerifyBackupWithMissingFile(primary);
			backupRestoreHelper.VerifyBackupWithExtraFile(primary);

            replicator->ResetQueue();
            replicator->Close();
            primary->Close();
        }

        // Cleanup previous primary directory.
        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabasePrimaryDirectory, true).IsSuccess());

        {
            ComPointer<ComMockReplicator> newReplicator = make_com<ComMockReplicator>();
            PrimaryReplicaSPtr newPrimary = PrimaryReplica::Create(newReplicator);
            newPrimary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            newPrimary->UpdateEpoch(1, 0, 0);

            backupRestoreHelper.VerifyRestoreFromBackupChain(newPrimary, origData1, origData2);

            /*auto restoredData = newPrimary->ReadUserData();
            newPrimary->VerifyEqualData(origData1, restoredData, 0, numeric_limits<_int64>::max());*/

            newReplicator->Close();
            newPrimary->Close();
        }
    }

    BOOST_AUTO_TEST_CASE(LogTruncationTimerTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** LogTruncationTimerTest ***");

        BackupRestoreHelper backupRestoreHelper;

        {
            ReplicatedStoreSettings replicatedStoreSettings;
            replicatedStoreSettings.LogTruncationInterval = TimeSpan::FromSeconds(15);

            auto replicator = make_com<ComMockReplicator>();

            SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);
            secondary->UpdateSettings(replicatedStoreSettings);
            secondary->Open(::FABRIC_REPLICA_ROLE_NONE, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            secondary->Store.Test_SetLogTruncationTestMode(true);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
            primary->UpdateSettings(replicatedStoreSettings);
            primary->Open(::FABRIC_REPLICA_ROLE_NONE, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            primary->Store.Test_SetLogTruncationTestMode(true);
            primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);

            backupRestoreHelper.InsertRandomData(primary);
            auto error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Full);
            CODING_ERROR_ASSERT(error.IsSuccess());

            for (int iter = 1; iter <= 6; iter++)
            {
                Sleep(10000);

                backupRestoreHelper.InsertRandomData(primary);
                error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Incremental);
                CODING_ERROR_ASSERT(error.IsSuccess());
            }

            Sleep(2000);

            CODING_ERROR_ASSERT(secondary->Store.Test_IsLogTruncationTimerSet() == true);
            CODING_ERROR_ASSERT(secondary->Store.Test_GetLogTruncationTimerFireCount() == 4);

            CODING_ERROR_ASSERT(primary->Store.Test_IsLogTruncationTimerSet() == true);
            CODING_ERROR_ASSERT(primary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            Sleep(16000);

            CODING_ERROR_ASSERT(secondary->Store.Test_GetLogTruncationTimerFireCount() == 5);
            CODING_ERROR_ASSERT(primary->Store.Test_GetLogTruncationTimerFireCount() == 1);

            backupRestoreHelper.InsertRandomData(primary);
            error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Incremental);
            CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::MissingFullBackup));

            replicator->ResetQueue();
            replicator->Close();
            primary->Close();
            secondary->Close();
        }

        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabasePrimaryDirectory, true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabaseSecondaryDirectory, true).IsSuccess());

        {
            ReplicatedStoreSettings replicatedStoreSettings;
            replicatedStoreSettings.LogTruncationInterval = TimeSpan::FromSeconds(0);

            auto replicator = make_com<ComMockReplicator>();

            SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);
            secondary->UpdateSettings(replicatedStoreSettings);
            secondary->Open(::FABRIC_REPLICA_ROLE_NONE, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            secondary->Store.Test_SetLogTruncationTestMode(true);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
            primary->UpdateSettings(replicatedStoreSettings);
            primary->Open(::FABRIC_REPLICA_ROLE_NONE, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            primary->Store.Test_SetLogTruncationTestMode(true);
            primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);

            Sleep(5000);

            CODING_ERROR_ASSERT(secondary->Store.Test_IsLogTruncationTimerSet() == false);
            CODING_ERROR_ASSERT(secondary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            CODING_ERROR_ASSERT(primary->Store.Test_IsLogTruncationTimerSet() == false);
            CODING_ERROR_ASSERT(primary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            replicator->ResetQueue();
            replicator->Close();
            primary->Close();
            secondary->Close();
        }

        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabasePrimaryDirectory, true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabaseSecondaryDirectory, true).IsSuccess());

        {
            ReplicatedStoreSettings replicatedStoreSettings;
            replicatedStoreSettings.LogTruncationInterval = TimeSpan::FromSeconds(5);

            auto replicator = make_com<ComMockReplicator>();

            SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);
            secondary->UpdateSettings(replicatedStoreSettings);
            secondary->Open(::FABRIC_REPLICA_ROLE_NONE, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
            secondary->Store.Test_SetLogTruncationTestMode(true);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

            Sleep(8000);

            CODING_ERROR_ASSERT(secondary->Store.Test_IsLogTruncationTimerSet() == false);
            CODING_ERROR_ASSERT(secondary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            replicator->ResetQueue();
            replicator->Close();
            secondary->Close();
        }

        CODING_ERROR_ASSERT(Directory::Delete(ReplicatedStoreTest::DatabaseSecondaryDirectory, true).IsSuccess());

        {
            ReplicatedStoreSettings replicatedStoreSettings;
            replicatedStoreSettings.LogTruncationInterval = TimeSpan::FromSeconds(5);

            auto replicator = make_com<ComMockReplicator>();

            SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);
            secondary->UpdateSettings(replicatedStoreSettings);
            secondary->Open(::FABRIC_REPLICA_ROLE_NONE);
            secondary->Store.Test_SetLogTruncationTestMode(true);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
            primary->UpdateSettings(replicatedStoreSettings);
            primary->Open(::FABRIC_REPLICA_ROLE_NONE);
            primary->Store.Test_SetLogTruncationTestMode(true);
            primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);

            Sleep(8000);

            CODING_ERROR_ASSERT(secondary->Store.Test_IsLogTruncationTimerSet() == false);
            CODING_ERROR_ASSERT(secondary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            CODING_ERROR_ASSERT(primary->Store.Test_IsLogTruncationTimerSet() == false);
            CODING_ERROR_ASSERT(primary->Store.Test_GetLogTruncationTimerFireCount() == 0);

            replicator->ResetQueue();
            replicator->Close();
            primary->Close();
            secondary->Close();
        }
    }

	BOOST_AUTO_TEST_CASE(FirstBackupOnPrimaryTest)
	{
		Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FirstBackupOnPrimaryTest ***");

		BackupRestoreHelper backupRestoreHelper;

		{
			ComPointer<ComMockReplicator> replicator = make_com<ComMockReplicator>();
			PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
			primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_NEW, true);
			primary->UpdateEpoch(1, 0, 0);

			backupRestoreHelper.InsertRandomData(primary);

			auto error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Incremental);
			CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::MissingFullBackup));

			for (int iter = 1; iter <= 3; iter++)
			{
				error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Full);
				CODING_ERROR_ASSERT(error.IsSuccess());

				error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Incremental);
				CODING_ERROR_ASSERT(error.IsSuccess());

				replicator->ResetQueue();
				primary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

				replicator->ResetQueue();
				primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

				error = backupRestoreHelper.TakeBackup(primary, StoreBackupOption::Incremental);
				CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::MissingFullBackup));
			}

			replicator->ResetQueue();
			replicator->Close();
			primary->Close();
		}
	}
#endif

#endif

    // Fault replication apply on the secondary to trigger retry logic.
    // Database contents should match afterwards.
    //
    BOOST_AUTO_TEST_CASE(ReplicationFaultApplyTest)
    {
        StoreConfig::GetConfig().SecondaryApplyRetryCount = 100;

        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ReplicationFaultApplyTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->VerifyProgress(1);

        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->VerifyProgress(3);

        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);

        // Drain replication queue to secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        secondary->Store.Test_SetSecondaryApplyFaultInjectionEnabled(true);

        replicator->QueueLastCopyOperation();

        // Inject faults
        //
        ManualResetEvent faultEvent(false);
        bool drainComplete = false;
        Threadpool::Post([&faultEvent, &drainComplete]
        {
            while (!drainComplete)
            {
                FaultInjector::GetGlobalFaultInjector().InjectTransientError(FaultInjector::ReplicatedStoreApplyTag, ErrorCodeValue::StoreWriteConflict);
#if !defined(PLATFORM_UNIX)
                Sleep(static_cast<DWORD>(StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds * 2));
#else
                Sleep(static_cast<DWORD>(StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds * 5));
#endif
            }

            faultEvent.Set();
        });

        replicator->DrainAndClose();

        // Give some time for retries to commit
        //
        Sleep(static_cast<DWORD>(StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds * 20));

        drainComplete = true;
        faultEvent.WaitOne();

        secondary->VerifyFaultCount();

        secondary->SetCopyNotificationComplete();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        // Ensure we are in the secondary role to cover closing from this state
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Fault replication apply on the secondary to trigger exhaustion of retry count.
    // Should report fault.
    //
    BOOST_AUTO_TEST_CASE(ReplicationFaultApplyTest2)
    {
        StoreConfig::GetConfig().SecondaryApplyRetryCount = 100;

        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ReplicationFaultApplyTest2({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->VerifyProgress(1);

        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->VerifyProgress(3);

        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);

        // Drain replication queue to secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        secondary->Store.Test_SetSecondaryApplyFaultInjectionEnabled(true);

        replicator->QueueLastCopyOperation();

        // Inject faults
        //
        FaultInjector::GetGlobalFaultInjector().InjectPermanentError(FaultInjector::ReplicatedStoreApplyTag, ErrorCodeValue::StoreWriteConflict);

        ManualResetEvent faultEvent(false);

        StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds = 100;
        StoreConfig::GetConfig().SecondaryApplyRetryCount = 5;

        Threadpool::Post([&faultEvent, &secondary]
        {
            while (secondary->GetFaultCount() == 0)
            {
                Sleep(StoreConfig::GetConfig().SecondaryApplyRetryDelayMilliseconds);
            }

            faultEvent.Set();
        });

        faultEvent.WaitOne();

        secondary->VerifyFaultCount(1);

        replicator->ResetQueue();

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Multiple updates to the same key within the same transaction is supported.
    // The value of the last update should take effect.
    //
    BOOST_AUTO_TEST_CASE(MultiUpdateReplicationTest)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** MultiUpdateReplicationTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueLastCopyOperation();

        primary->InsertData(typeCount, keyCount, bytesCount);

        {
            replicator->DrainAndClose();

            secondary->SetCopyNotificationComplete();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(1);

            secondary->Close();
            replicator->ResetQueue();
            secondary = SecondaryReplica::Create(replicator);
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
            replicator->QueueLastCopyOperation();
        }

        // Update with a larger byte count
        {
            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->InnerWriteData(
                txSPtr, 
                typeCount,
                keyCount,
                10, // byte count
                1,
                primary->DefaultTypePrefix,
                primary->DefaultKeyPrefix,
                ReplicationOperationType::Update);

            primary->InnerWriteData(
                txSPtr, 
                typeCount,
                keyCount,
                20, // byte count
                1,
                primary->DefaultTypePrefix,
                primary->DefaultKeyPrefix,
                ReplicationOperationType::Update);

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        auto type = wformatString("{0}{1}", primary->DefaultTypePrefix, 0);
        auto key = wformatString("{0}{1}", primary->DefaultKeyPrefix, 0);

        {
            replicator->DrainAndClose();

            secondary->SetCopyNotificationComplete();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(2);

            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr, type, key, enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = enumSPtr->MoveNext();
            VERIFY_IS_TRUE(error.IsSuccess());

            size_t valueSize = 0;
            error = enumSPtr->CurrentValueSize(valueSize);
            VERIFY_IS_TRUE(error.IsSuccess());

            // Last update should take effect
            //
            VERIFY_IS_TRUE(valueSize == 20);

            secondary->Close();
            replicator->ResetQueue();
            secondary = SecondaryReplica::Create(replicator);
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
            replicator->QueueLastCopyOperation();
        }

        // Update with a smaller byte count
        {
            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->InnerWriteData(
                txSPtr, 
                typeCount,
                keyCount,
                16, // byte count
                1,
                primary->DefaultTypePrefix,
                primary->DefaultKeyPrefix,
                ReplicationOperationType::Update);

            primary->InnerWriteData(
                txSPtr, 
                typeCount,
                keyCount,
                64, // byte count
                1,
                primary->DefaultTypePrefix,
                primary->DefaultKeyPrefix,
                ReplicationOperationType::Update);

            primary->InnerWriteData(
                txSPtr, 
                typeCount,
                keyCount,
                32, // byte count
                1,
                primary->DefaultTypePrefix,
                primary->DefaultKeyPrefix,
                ReplicationOperationType::Update);

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {
            replicator->DrainAndClose();

            secondary->SetCopyNotificationComplete();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(3);

            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr, type, key, enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = enumSPtr->MoveNext();
            VERIFY_IS_TRUE(error.IsSuccess());

            size_t valueSize = 0;
            error = enumSPtr->CurrentValueSize(valueSize);
            VERIFY_IS_TRUE(error.IsSuccess());

            // Last update should take effect
            //
            VERIFY_IS_TRUE(valueSize == 32);
        }

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    BOOST_AUTO_TEST_CASE(LargeTransactionTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** LargeTransactionTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueLastCopyOperation();

        wstring dataType(L"LargeTransactionTestType");
        wstring dataKeyPrefix(L"LargeTransactionTestKey_");

        int keyCount = 10000;
        vector<byte> data;
        data.push_back(static_cast<byte>(0));

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            Stopwatch stopwatch;

            stopwatch.Start();

            for (auto ix=0; ix<keyCount; ++ix)
            {
                error = primary->Store.Insert(
                    txSPtr, 
                    dataType,
                    wformatString("{0}{1}", dataKeyPrefix, ix), 
                    data.data(), 
                    data.size());
                VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success));
            }

            stopwatch.Stop();

            auto throughput = (double)keyCount / stopwatch.ElapsedMilliseconds * 1000;

            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource,
                "Insert: count={0} elapsed={1} througput={2} keys/sec",
                keyCount,
                stopwatch.Elapsed,
                throughput);
            
            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            Stopwatch stopwatch;

            stopwatch.Start();

            for (auto ix=0; ix<keyCount; ++ix)
            {
                error = primary->Store.Delete(
                    txSPtr, 
                    dataType,
                    wformatString("{0}{1}", dataKeyPrefix, ix));
                VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Success));
            }

            stopwatch.Stop();

            auto throughput = (double)keyCount / stopwatch.ElapsedMilliseconds * 1000;

            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource,
                "Delete: count={0} elapsed={1} througput={2} keys/sec",
                keyCount,
                stopwatch.Elapsed,
                throughput);
            
            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Verify database directory clean-up on ChangeRole to None
    //
    BOOST_AUTO_TEST_CASE(ChangeRoleNoneTest)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ChangeRoleNoneTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->VerifyProgress(1);

        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->VerifyProgress(3);

        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);

        // Drain replication queue to secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        wstring secondaryDbDirectory;
        secondary->CreateDatabaseDirectoryName(secondaryDbDirectory);

        wstring primaryDbDirectory;
        primary->CreateDatabaseDirectoryName(primaryDbDirectory);

        VERIFY_IS_TRUE(Directory::Exists(secondaryDbDirectory));
        VERIFY_IS_TRUE(Directory::Exists(primaryDbDirectory));

        // Change role to none
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

        replicator->Close();
        secondary->Close();
        primary->Close();

        // Secondary directory should be deleted at this point but not primary
        VERIFY_IS_FALSE(Directory::Exists(secondaryDbDirectory));
        VERIFY_IS_TRUE(Directory::Exists(primaryDbDirectory));
    }

    // Same as ReplicationTest but complete the replications asynchronously to cover the code path
    // in TransactionReplicator where CompletedSynchronously == false
    //
    BOOST_AUTO_TEST_CASE(AsynchronousReplicationTest)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** AsynchronousReplicationTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        // Set mock replicator to switch threads before completing replication
        auto replicator = make_com<ComMockReplicator>(false);
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Fill replication queue
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->VerifyProgress(1);

        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->VerifyProgress(3);

        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);

        // Drain replication queue to secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        // Ensure we are in the secondary role to cover closing from this state
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Populate a secondary with data and failover to become a primary.
    // The new primary should be able to replicate to another secondary
    // with the same state that the new primary had before the failover occurred.
    //
    BOOST_AUTO_TEST_CASE(FailoverTest)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FailoverTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Replicate data from primary to secondary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);
        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        size_t dataCount = dataToVerify.size();

        // Failover secondary into primary role
        // Swap primary out into secondary role
        // Replicate from new primary to new secondary
        //
        std::swap(primary, secondary);
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        primary->UpdateEpoch(2, 0, 0);
        secondary->UpdateEpoch(2, 0, 0);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 6);

        wstring failoverTypePrefix(L"FailoverType_");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, failoverTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, failoverTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5, failoverTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, failoverTypePrefix);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3, failoverTypePrefix);
        primary->DeleteData(typeCount, keyCount, 7, failoverTypePrefix);
        replicator->DrainAndClose();

        // Database contents should still be identical and contain data items replicated
        // after the failover
        // 
        dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        VERIFY_IS_TRUE(dataToVerify.size() == (dataCount * 2));

        primary->VerifyProgress(13);
        secondary->VerifyProgress(13);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Replicated operations can commit on the primary before they are quorum-acked. However, the operations
    // can only commit in increasing replication sequence number order
    //
    BOOST_AUTO_TEST_CASE(SerialCommit)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** SerialCommit({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        // Multi-threaded replication to cause racing commits
        //
        int totalOperationCount = 100;
        Common::atomic_long completionCount(static_cast<LONG>(totalOperationCount));
        ManualResetEvent waiter(false);

        for (int ix = 0; ix < totalOperationCount; ++ix)
        {
            std::wstring typePrefix;
            StringWriter writer(typePrefix);
            writer.Write("Type{0}_", ix);

            Threadpool::Post([&, typePrefix]() 
            { 
                primary->InsertData(typeCount, keyCount, bytesCount, 1, typePrefix); 

                if (--completionCount == 0)
                {
                    waiter.Set();
                }
            });
        }

#if !defined(PLATFORM_UNIX)
        // Wait for all operations to complete on primary
        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));
#else
        // On Linux, due to performance difference, this test might not be able to finish in 10 sec
        // have to increase the timeout.
        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(3 * WaitOneTimeoutInSeconds)));
#endif

        // Primary and secondary database contents should be identical
        // 
        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        primary->VerifyProgress(totalOperationCount);
        secondary->VerifyProgress(totalOperationCount);

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

#if !defined(PLATFORM_UNIX)
    // Similar to SerialCommit, but injects an artificial local commit delay for more targeted testing
    //
    BOOST_AUTO_TEST_CASE(SerialCommit2)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** SerialCommit2({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>(false, 1); // async replication, 1 ms delay
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        TimeSpan localApplyDelay = TimeSpan::FromSeconds(5);

        vector<byte> dummy; 
        dummy.push_back(42);

        size_t operationCount = 2;

        vector<shared_ptr<ManualResetEvent>> events;
        for (auto ix=0; ix<operationCount; ++ix)
        {
            events.push_back(make_shared<ManualResetEvent>(false));
        }

        vector<TransactionSPtr> txs;
        for (auto ix=0; ix<operationCount; ++ix)
        {
            TransactionSPtr tx;
            auto error = primary->Store.CreateTransaction(tx);
            VERIFY_IS_TRUE(error.IsSuccess());

            txs.push_back(tx);
        }

        for (auto ix=0; ix<operationCount; ++ix)
        {
            auto error = primary->Store.Insert(
                txs[ix], 
                L"SerialCommitType", 
                wformatString("SerialCommitKey{0}", ix), 
                dummy.data(), 
                dummy.size());
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        for (auto ix=0; ix<operationCount; ++ix)
        {
            if (ix == 0)
            {
                primary->Store.Test_SetLocalApplyDelay(localApplyDelay);
            }

            txs[ix]->BeginCommit(
                TimeSpan::MaxValue,
                [ix, &txs, &events](AsyncOperationSPtr const & operation) 
                { 
                    FABRIC_SEQUENCE_NUMBER lsn = 0;
                    auto error = txs[ix]->EndCommit(operation, lsn);
                    VERIFY_IS_TRUE(error.IsSuccess());

                    events[ix]->Set(); 
                },
                primary->CreateAsyncOperationRoot());

            if (ix == 0)
            {
                Sleep(static_cast<DWORD>(localApplyDelay.TotalMilliseconds() / 2));

                primary->Store.Test_SetLocalApplyDelay(TimeSpan::Zero);
            }
        }

        events[operationCount-1]->WaitOne();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();

        VERIFY_IS_TRUE(dataToVerify.size() == operationCount);

        primary->VerifyProgress(dataToVerify.size());

        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(dataToVerify.size());

        txs.clear();

        replicator->Close();
        secondary->Close();
        primary->Close();
    }
#endif


    // Test that the replicated store pumps (state machine and secondary pump) can continue to
    // pump even when the threadpool is saturated.
    //
    BOOST_AUTO_TEST_CASE(ThreadpoolDeadlock)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ThreadpoolDeadlock({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        // Multi-threaded replication to exceed max threadpool limit (default is 500 threads)
        //
        int totalOperationCount = 1000;
        Common::atomic_long completionCount(static_cast<LONG>(totalOperationCount));
        ManualResetEvent waiter(false);

        for (int ix = 0; ix < totalOperationCount; ++ix)
        {
            Threadpool::Post([&]() 
            { 
                TransactionSPtr tx;
                ErrorCode error = primary->Store.CreateTransaction(tx);
                VERIFY_IS_TRUE(error.IsSuccess());

                if (--completionCount == 0)
                {
                    waiter.Set();
                }
            });
        }

        // Wait for all operations to complete on primary
        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds * 10)));

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // ReplicatedStore should be aborted if it fails to open
    //
#if !defined(PLATFORM_UNIX) // Disabling because test case is crashing..
    BOOST_AUTO_TEST_CASE(AbortTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** AbortTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr conflictingPrimary = PrimaryReplica::Create(replicator);
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

        // Intentionally cause open to fail by opening the database file
        conflictingPrimary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::StoreInUse, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Cannot create transactions on aborted replica
        TransactionSPtr txSPtr;
        ErrorCode error = primary->Store.CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));

        replicator->Close();
        conflictingPrimary->Close();
    }
#endif

    // ReplicatedStore should best-effort cleanup on abort to allow
    // re-open of the replica
    //
    BOOST_AUTO_TEST_CASE(AbortReOpenTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** AbortReOpenTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        primary->Abort();

        PrimaryReplicaSPtr reopenPrimary = PrimaryReplica::Create(replicator);
        reopenPrimary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        {
            TransactionSPtr txSPtr;
            ErrorCode error = reopenPrimary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        replicator->Close();
        reopenPrimary->Close();
    }

    // Transactions that fail to initialize when the underlying local
    // store is faulted must not count towards outstanding transactions
    // (a subsequent change-role/close must not be blocked)
    //
    BOOST_AUTO_TEST_CASE(FaultedStoreTransactionTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FaultedStoreTransactionTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        // Intentionally fault the underlying store without closing replica
        //
        {
            auto error = primary->Store.LocalStore->Cleanup();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        // Transaction creation should fail
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(!error.IsSuccess());
        }

        replicator->DrainAndClose();
        primary->Close();
        secondary->Close();
    }

    // Test that transactions get aborted after encountering an error.
    //
    BOOST_AUTO_TEST_CASE(TransactionAbort)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TransactionAbort");

        vector<byte> data; 
        data.push_back(42);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        // Invalid type
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                Constants::ProgressDataType,
                L"RollbackKey", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::InvalidArgument));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));
        }

        // Exception: StoreWriteConflict will not abort the transaction
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey",
                0); 
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::StoreWriteConflict));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType2", 
                L"RollbackKey2", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());
        }
        
        // Exception: StoreRecordAlreadyExists will not abort the transaction
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                L"AlreadyExistsType", 
                L"AlreadyExistsType", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                L"AlreadyExistsType", 
                L"AlreadyExistsType", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::StoreRecordAlreadyExists));

            error = primary->Store.Insert(
                txSPtr, 
                L"AlreadyExistsType2", 
                L"AlreadyExistsType2", 
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                L"AlreadyExistsType2", 
                L"AlreadyExistsType2", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::StoreRecordAlreadyExists));

            error = primary->Store.Update(
                txSPtr, 
                L"AlreadyExistsType", 
                L"AlreadyExistsType", 
                0,
                L"AlreadyExistsType", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());
        }
        
        // Exception: StoreRecordNotFound will not abort the transaction
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey",
                0);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::StoreRecordNotFound));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType2", 
                L"RollbackKey2", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());
        }


        // Exception: EnumerationCompleted will not abort transaction
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = enumSPtr->MoveNext();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::EnumerationCompleted));

            error = enumSPtr->MoveNext();
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::EnumerationCompleted));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType2", 
                L"RollbackKey2", 
                data.data(), 
                data.size());
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        primary->SetAccessDenied();
        
        // Write reconfiguration pending
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ReconfigurationPending));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType2", 
                L"RollbackKey2", 
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                enumSPtr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));
        }
        
        // Read reconfiguration pending
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                enumSPtr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ReconfigurationPending));

            error = primary->Store.CreateEnumerationByTypeAndKey(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                enumSPtr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));

            error = primary->Store.CreateEnumerationByTypeAndKey(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                enumSPtr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));

            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType2", 
                L"RollbackKey2", 
                data.data(),
                data.size());
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::TransactionAborted));
        }
        
        replicator->DrainAndClose();

        primary->Close();
        secondary->Close();
    }

    // Test that rolling back transactions functions correctly
    //
    BOOST_AUTO_TEST_CASE(TransactionRollback)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TransactionRollback({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        primary->InsertData(typeCount, keyCount, bytesCount, 1);
        primary->VerifyProgress(1);

        // Rollback a write
        //
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            vector<byte> dummy; 
            dummy.push_back(42);
            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                dummy.data(),
                dummy.size());
            VERIFY_IS_TRUE(error.IsSuccess());

            txSPtr->Rollback();
        }
        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        auto primaryData = primary->ReadUserData();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->VerifyData(primaryData);
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(typeCount * keyCount));

        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);

        // Destruct transactions - should automatically rollback
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        {
            TransactionSPtr txSPtr;
            TransactionSPtr tx2SPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.CreateTransaction(tx2SPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            vector<byte> dummy; 
            dummy.push_back(42);
            error = primary->Store.Insert(
                txSPtr, 
                L"RollbackType", 
                L"RollbackKey", 
                dummy.data(),
                dummy.size());
            VERIFY_IS_TRUE(error.IsSuccess());

#if !defined(PLATFORM_UNIX) //Multiple write using transaction is not supported in case of sqlite
            error = primary->Store.Insert(
                tx2SPtr, 
                L"Rollback2Type", 
                L"Rollback2Key", 
                dummy.data(),
                dummy.size());
            VERIFY_IS_TRUE(error.IsSuccess());
#endif
        }
        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        primaryData = primary->ReadUserData();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->VerifyData(primaryData);
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(typeCount * keyCount));

        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);

        primary->Close();
        secondary->Close();
    }

    // Test that outstanding valid transactions will prevent
    // the replicated store from destructing.
    //
    BOOST_AUTO_TEST_CASE(TransactionLifetime)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TransactionLifetime({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();

        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        PrimaryReplica * primaryPtr = NULL;
        TransactionSPtr validTxSPtr;
        {
            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            replicator->QueueLastCopyOperation();

            primary->InsertData(typeCount, keyCount, bytesCount, 1);
            primary->VerifyProgress(1);

            // the outstanding transaction should prevent the replica from destructing
            // after leaving this scope
            //
            primaryPtr = dynamic_cast<PrimaryReplica*>(primary.get());
            ErrorCode error = primaryPtr->Store.CreateTransaction(validTxSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        vector<byte> dummy; 
        dummy.push_back(42);
        ErrorCode error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey1", 
            L"LifetimeKey1", 
            dummy.data(),
            dummy.size());
        VERIFY_IS_TRUE(error.IsSuccess());

        error = validTxSPtr->Commit();
        VERIFY_IS_TRUE(error.IsSuccess());

        replicator->DrainAndClose();

        AutoResetEvent waiter(false);

        primaryPtr->Store.BeginClose(
            [this, &waiter, &primaryPtr](AsyncOperationSPtr const & operation) 
            { 
                ErrorCode error = primaryPtr->Store.EndClose(operation);
                VERIFY_IS_TRUE(error.IsSuccess());
                waiter.Set(); 
            },
            primaryPtr->CreateAsyncOperationRoot());

        // the outstanding transaction (whether valid or not) should not prevent the replica from closing
        //
        TransactionSPtr invalidTxSPtr;
        error = primaryPtr->Store.CreateTransaction(invalidTxSPtr);
        bool success = (error.IsError(ErrorCodeValue::NotPrimary) || error.IsError(ErrorCodeValue::Success) || error.IsError(ErrorCodeValue::ObjectClosed));
        VERIFY(success, "error={0}", error);

        validTxSPtr.reset();

        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"Primary::Close(): timeout");

        secondary->Close();
    }

    // The last valid transaction keeping the underlying ESE local store alive (via replicated store)
    // should destruct and decrement the pending transaction count correctly. The ESE local store
    // will assert against an incorrect transaction count during destruction.
    //
    BOOST_AUTO_TEST_CASE(TransactionLifetime2)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TransactionLifetime2({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();

        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        PrimaryReplica * primaryPtr = NULL;
        TransactionSPtr validTxSPtr;
        {
            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            replicator->QueueLastCopyOperation();

            primary->InsertData(typeCount, keyCount, bytesCount, 1);
            primary->VerifyProgress(1);

            // the outstanding transaction should prevent the replica from destructing
            // after leaving this scope
            //
            primaryPtr = dynamic_cast<PrimaryReplica*>(primary.get());
            ErrorCode error = primaryPtr->Store.CreateTransaction(validTxSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        vector<byte> dummy; 
        dummy.push_back(42);
        ErrorCode error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey1", 
            L"LifetimeKey1", 
            dummy.data(),
            dummy.size());
        VERIFY_IS_TRUE(error.IsSuccess());

        error = validTxSPtr->Commit();
        VERIFY_IS_TRUE(error.IsSuccess());

        replicator->DrainAndClose();

        // the active transaction count should decrement before the underlying store is destructed
        //
        AutoResetEvent waiter(false);

        primaryPtr->Store.BeginClose(
            [this, &waiter, &primaryPtr](AsyncOperationSPtr const & operation) 
            { 
                ErrorCode error = primaryPtr->Store.EndClose(operation);
                VERIFY_IS_TRUE(error.IsSuccess());
                waiter.Set(); 
            },
            primaryPtr->CreateAsyncOperationRoot());

        validTxSPtr.reset();

        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"Primary::Close(): timeout");

        secondary->Close();
    }

    // Tests the functionality of forcefully releasing transactions when the primary demotes or closes.
    // When entering the PrimaryChangePending state, the store will abort any transactions still outstanding.
    // ChangeRole and Close should not be blocked by outstanding transaction.
    //
    BOOST_AUTO_TEST_CASE(TransactionLifetime3)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        StoreConfig::GetConfig().TransactionDrainTimeout = TimeSpan::FromSeconds(60);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TransactionLifetime3({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();

        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        PrimaryReplica * primaryPtr = NULL;
        TransactionSPtr validTxSPtr;
        {
            PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
            replicator->QueueLastCopyOperation();

            primary->InsertData(typeCount, keyCount, bytesCount, 1);
            primary->VerifyProgress(1);

            // the outstanding transaction should prevent the replica from destructing
            // after leaving this scope
            //
            primaryPtr = dynamic_cast<PrimaryReplica*>(primary.get());
            ErrorCode error = primaryPtr->Store.CreateTransaction(validTxSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        vector<byte> dummy; 
        dummy.push_back(42);
        ErrorCode error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey", 
            L"LifetimeKey1", 
            dummy.data(), 
            dummy.size());
        VERIFY_IS_TRUE(error.IsSuccess());

        // Outstanding transaction should not block change role
        //
        replicator->DrainAndClose();
        secondary->Close();
        primaryPtr->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        // Outstanding transactions should be automatically aborted
        //
        error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey", 
            L"LifetimeKey2", 
            dummy.data(), 
            dummy.size());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        TransactionSPtr invalidTxSPtr;
        error = primaryPtr->Store.CreateTransaction(invalidTxSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        // Replica can still be promoted back to primary
        //
        replicator->DrainAndClose();
        primaryPtr->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Old outstanding transaction is still aborted
        //
        error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey", 
            L"LifetimeKey2", 
            dummy.data(), 
            dummy.size());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        // Create some new transactions
        //
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Creating tx");
        TransactionSPtr validTxSPtr2;
        error = primaryPtr->Store.CreateTransaction(validTxSPtr2);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Checking result");
        VERIFY_IS_TRUE(error.IsSuccess());

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Inserting LifetimeKey3");

        error = primaryPtr->Store.Insert(
            validTxSPtr2, 
            L"LifetimeKey", 
            L"LifetimeKey3", 
            dummy.data(), 
            dummy.size());
        VERIFY_IS_TRUE(error.IsSuccess());

        TransactionSPtr validTxSPtr3;
        error = primaryPtr->Store.CreateTransaction(validTxSPtr3);
        VERIFY_IS_TRUE(error.IsSuccess());

        error = primaryPtr->Store.Insert(
            validTxSPtr3, 
            L"LifetimeKey", 
            L"LifetimeKey4", 
            dummy.data(), 
            dummy.size());
        VERIFY_IS_TRUE(error.IsSuccess());

        // Replica can still close with outstanding transactions
        //
        primaryPtr->Close();

        // Outstanding transactions are aborted
        //
        error = primaryPtr->Store.Insert(
            validTxSPtr, 
            L"LifetimeKey", 
            L"LifetimeKey2", 
            dummy.data(),
            dummy.size());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        error = primaryPtr->Store.Insert(
            validTxSPtr2, 
            L"LifetimeKey", 
            L"LifetimeKey5", 
            dummy.data(),
            dummy.size());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        error = primaryPtr->Store.Insert(
            validTxSPtr3, 
            L"LifetimeKey", 
            L"LifetimeKey6", 
            dummy.data(),
            dummy.size());
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        replicator->DrainAndClose();
        secondary->Close();

        // Transactions are still outstanding but aborted and released.
        // Should be able to re-open a replica on the same store.
        //
        auto primary2 = PrimaryReplica::Create(replicator);

        primary2->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary2->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

        TransactionSPtr reopenTxSPtr;
        error = primary2->Store.CreateTransaction(reopenTxSPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        TransactionSPtr closedTxSPtr;
        error = primaryPtr->Store.CreateTransaction(closedTxSPtr);
        bool success = (error.IsError(ErrorCodeValue::ObjectClosed) || error.IsError(ErrorCodeValue::NotPrimary));
        VERIFY(success, "error={0}", error);

        primary2->Close();
    }

    // Tests that enumerations can destruct after their transactions without triggering asserts.
    // This test intentionally covers code paths that will TestAssert so do not
    // run it unless test asserts are disabled.
    //
    BOOST_AUTO_TEST_CASE(EnumerationLifetime)
    {
        if (Assert::IsTestAssertEnabled())
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Test assert enabled: skipping EnumerationLifetime test");
            return;
        }

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** EnumerationLifetime");

        auto replicator = make_com<ComMockReplicator>();

        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);

        weak_ptr<IStoreBase::TransactionBase> txWPtr;
        IStoreBase::EnumerationSPtr enumSPtr;
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(txSPtr.use_count() == 1);

            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr, L"type", L"key", enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());
            VERIFY_IS_TRUE(txSPtr.use_count() == 1);
            VERIFY_IS_TRUE(enumSPtr.use_count() == 1);

            txWPtr = txSPtr;

            VERIFY_IS_TRUE(txSPtr.use_count() == 1);
        }

        VERIFY_IS_TRUE(txWPtr.use_count() == 0);
        VERIFY_IS_TRUE(enumSPtr.use_count() == 1);

        VERIFY_IS_TRUE(enumSPtr->MoveNext().IsError(ErrorCodeValue::StoreTransactionNotActive));

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    // Full copy data from a primary to a blank secondary.
    // Databases should match afterwards.
    //
    BOOST_AUTO_TEST_CASE(FullCopyTest1)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyTest1({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->UpdateData(typeCount, keyCount, bytesCount * 5, 5);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 3, 3);
        primary->DeleteData(typeCount, keyCount, 7);
        primary->VerifyProgress(6);

        // Create empty secondary
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->DrainAndClose();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == 0);

        // copy everything from primary to blank secondary
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        ::FABRIC_SEQUENCE_NUMBER operationLSN = replicator->LastOperationLSN;
        replicator->QueueCopyOperations(primary->Store, secondary->Store, operationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Full copy data from a primary to a non-empty secondary.
    // All data on the secondary should be overwritten.
    // 
    BOOST_AUTO_TEST_CASE(FullCopyTest2)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyTest2({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->VerifyProgress(3);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data but do NOT
        // update the epoch. The secondary will fail to report
        // any copy context - resulting in a full copy 
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->InsertData(typeCount, keyCount * 2, bytesCount * 2);
        secondary->UpdateData(typeCount, keyCount * 2, bytesCount * 4, 2);
        secondary->DeleteData(typeCount, keyCount, 3);
        secondary->InsertData(typeCount, keyCount, bytesCount * 6, 3);
        secondary->DeleteData(typeCount, keyCount, 4);
        secondary->VerifyProgress(8);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() != secondaryData.size());

        // copy everything from primary to secondary
        // (should overwrite all data on secondary)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(3);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Full copy of large data items
    //
    BOOST_AUTO_TEST_CASE(FullCopyTest3)
    {
        int typeCount = 2;
        int keyCount = 2;
        int bytesCount = static_cast<int>(StoreConfig::GetConfig().TargetCopyOperationSize * 4);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyTest3({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->VerifyProgress(3);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Create empty secondary
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->DrainAndClose();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == 0);

        // copy everything from primary to secondary
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(3);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Request copy operations that haven't committed on the primary yet.
    // Since there is no "retry" contract with the replication layer, 
    // ComCopyOperationContext needs to wait internally until the requested copy
    // operations have committed.
    //
    BOOST_AUTO_TEST_CASE(FullCopyTest4)
    {
        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyTest4({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->InsertData(typeCount, keyCount, bytesCount * 2, 1, L"Type2_");
        primary->DeleteData(typeCount, keyCount, 2, L"Type2_");
        primary->VerifyProgress(3);

        // Create a new (empty) secondary
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

        DateTime start = DateTime::Now();
        Threadpool::Post([&primary]()
        {            
            primary->InsertData(1, 1, 1, 1, L"Type3_");
        },
            TimeSpan::FromMilliseconds(1500));

        // Request copy operations for some data that hasn't committed yet.
        // Copy should not complete until the requested sequence number has 
        // been committed.
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4);
        DateTime stop = DateTime::Now();

        TimeSpan delay = stop - start;
        VERIFY(delay > TimeSpan::FromMilliseconds(1000), "delay = {0} is too small", delay);

        replicator->DrainAndClose();

        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(4);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        primary->Close();
        secondary->Close();
    }

    // Ensure that a secondary that was populated via copy operations can fail over
    // for the primary and perform further copying to another secondary
    //
    BOOST_AUTO_TEST_CASE(FullCopyFailoverTest)
    { 
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring extraType2Prefix(L"AnotherExtraType_");
        wstring extraType3Prefix(L"ReplicationType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyFailoverTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount * 4, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraType2Prefix);
        primary->VerifyProgress(7);

        // Full copy all of the data committed on the primary. Partially
        // built secondaries may not be consistent and cannot be used to
        // build other secondaries.
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7);
        replicator->DrainAndClose();

        // Failover secondary to become the new primary
        // and bring up a new blank secondary
        //
        std::swap(primary, secondary);
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->Close();
        secondary.reset();
        DeleteDirectory(DatabasePrimaryDirectory);
        Directory::Create(DatabasePrimaryDirectory);
        secondary = PrimaryReplica::Create(replicator);
        // Open in primary role to verify user data first
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);

        auto primaryData = primary->ReadUserData();
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() > 0);
        VERIFY_IS_TRUE(secondaryData.size() == 0);

        primary->VerifyProgress(7);
        secondary->VerifyProgress(0);

        // New primary should be able to copy operations to new secondary
        // and also replicate new operations.
        //
        replicator->ResetQueue();
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraType3Prefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraType3Prefix);
        primary->DeleteData(typeCount, keyCount, 3, extraType3Prefix);
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->DrainAndClose();

        // New primary and secondary database contents should be identical
        // 
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        // Check that the data item count is correct
        auto expectedCount = static_cast<size_t>((typeCount * keyCount) * 4);
        VERIFY(dataToVerify.size() == expectedCount, "count={0} expected={1}", dataToVerify.size(), expectedCount);

        auto progress = primary->ReadProgress();
        secondary->VerifyProgress(progress);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partially built secondaries may not be consistent and cannot be used to build other
    // secondaries. All builds should happen in a separate database on the side and
    // swap with the active database only after the entire build process completes.
    //
    // This tests the full copy protocol for versions < 4.2.
    //
    BOOST_AUTO_TEST_CASE(FullCopyInterruptionTest)
    { 
        int typeCount = 2;
        int keyCount = 5;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring extraType2Prefix(L"ExtraType2_");
        wstring falseProgressPrefix(L"FalseProgressType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyInterruptionTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        StoreConfig::GetConfig().EnableFileStreamFullCopy = false;

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        secondary->DeleteData(typeCount, keyCount, 3);
        secondary->InsertData(typeCount, keyCount, bytesCount * 4, 1, extraTypePrefix);
        // Last item will be false progress (not copied)
        secondary->InsertData(typeCount, keyCount * 2, bytesCount * 2, 1, falseProgressPrefix);
        secondary->VerifyProgress(5);

        // Copy all data to the primary except for the false progress,
        // which we will leave only on the secondary
        //
        replicator->ResetQueue();
        primary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(secondary->Store, primary->Store, 4);
        replicator->DrainAndClose();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Initialize");

        // Verify primary and secondary are identical at this point
        // except for the false progress
        //
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

        auto primaryData = primary->ReadUserData();
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() != secondaryData.size());

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Initialize Copy");

        // Simulate full copy interruption by copying some of the
        // primary's data to the secondary (do not queue the
        // last copy operation, which completes the copy).
        //
        primary->Close();
        secondary->Close();

        replicator->ResetQueue();

        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        
        primary->UpdateEpoch(2, 0, 4);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 2, ErrorCodeValue::Success, false);

        // do not close, which would pump null to the secondary
        replicator->Drain();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart primary 1");

        // Restart the primary only. The secondary will have an incomplete
        // full copy database already loaded, but still report progress 
        // using the last complete database. The primary will restart the full
        // copy.
        //
        primary->Close();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 1, ErrorCodeValue::Success, false);
        replicator->Drain();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart secondary 1");
        
        // Restart the secondary only. The secondary will again reset the
        // full copy database. The primary will restart the full
        // copy.
        //
        secondary->AsyncAbort();
        replicator->ResetQueue();

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3, ErrorCodeValue::Success, false);
        replicator->Drain();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart primary and secondary");
        
        // Restart both replicas. The secondary should still be reporting
        // progress from the last complete database.
        //
        primary->Close();
        secondary->AsyncAbort();
        replicator->ResetQueue();

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        // Add more data to primary
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraType2Prefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraType2Prefix);
        primary->DeleteData(typeCount, keyCount, 3, extraType2Prefix);

        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        // Copy enough data for the secondary full copy database to be more advanced
        // than its last complete database, but do not finish the copy.
        // The secondary should continue to report progress on the old data.
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 6, ErrorCodeValue::Success, false);
        replicator->DrainCopy();

        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart primary 2");

        // Restart primary again
        primary->Close();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 6, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        
        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart secondary 2");

        // Restart secondary again
        //
        secondary->AsyncAbort();
        replicator->ResetQueue();

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 12, ErrorCodeValue::Success, false);
        replicator->Drain();
        
        primary->VerifyProgress(12);
        secondary->VerifyProgress(5);

        replicator->QueueLastCopyOperation();

        // Finish the copy - databases should now match
        //
        replicator->DrainAndClose();

        primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        primary->VerifyProgress(12);
        secondary->VerifyProgress(12);

        progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Same semantics as FullCopyInterruptionTest but with the file stream
    // full copy protocol (>= 4.2)
    //
    BOOST_AUTO_TEST_CASE(FullCopyFileStreamInterruptionTest)
    { 
        int typeCount = 2;
        int keyCount = 5;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring falseProgressType(L"FalseProgressType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FullCopyFileStreamInterruptionTest({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        StoreConfig::GetConfig().TargetCopyOperationSize = bytesCount; // ensure file stream gets paged
#if defined(PLATFORM_UNIX)
        StoreConfig::GetConfig().EnableFileStreamFullCopy = false; // false for Linux
#else
        StoreConfig::GetConfig().EnableFileStreamFullCopy = true; // true is default, but set explicitly
#endif

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate secondary
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        secondary->DeleteData(typeCount, keyCount, 3);
        secondary->InsertData(typeCount, keyCount, bytesCount * 4, 1, extraTypePrefix);

        // Copy all data to the primary
        //
        replicator->ResetQueue();
        primary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(secondary->Store, primary->Store, 4);
        replicator->DrainAndClose();

        primary->VerifyProgress(4);
        secondary->VerifyProgress(4);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Initialize");

        // Add false progress to the secondary to trigger full copies
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs(4);

        primary->UpdateEpoch(3, 0, 4);
        secondary->UpdateEpoch(2, 0, 4);
        secondary->InsertData(typeCount, keyCount * 2, bytesCount * 2, 1, falseProgressType);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Initialize Copy");

        // Simulate full copy file stream interruption by
        // not queueing all copy operation chunks
        //
        primary->Close();
        secondary->Close();

        replicator->ResetQueue();

        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        
        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4, ErrorCodeValue::Success, false, 3);
        replicator->DrainCopy();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart primary 1");

        // Restart the primary only. The secondary should still have the old
        // database attached. The incomplete file stream will be discarded
        // when restarting the full copy.
        //
        primary->Close();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4, ErrorCodeValue::Success, false, 3);
        replicator->DrainCopy();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart secondary 1");
        
        // Restart the secondary only. The secondary will again reset the
        // full copy database. The primary will restart the full
        // copy.
        //
        secondary->AsyncAbort();
        replicator->ResetQueue();

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4, ErrorCodeValue::Success, false, 3);
        replicator->DrainCopy();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Restart primary and secondary");
        
        // Restart both replicas. The secondary should still be reporting
        // progress from the last complete database.
        //
        primary->Close();
        secondary->AsyncAbort();
        replicator->ResetQueue();

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(5);

        // Finish the copy - databases should now match
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4);
        replicator->DrainAndClose();

        auto primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        primary->VerifyProgress(4);
        secondary->VerifyProgress(4);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary.
    // Pre-existing data on the secondary should be NOT overwritten.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest1)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest1({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that is less advanced
        // than primary.
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 4, 2, recoveredTypePrefix);
        secondary->DeleteData(typeCount, keyCount, 3, recoveredTypePrefix);
        secondary->VerifyProgress(3);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (should not overwrite any data already on secondary)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database should NOT be identical.
        // Original data on secondary should be preserved and more
        // advanced primary data should have been copied.
        //
        secondary->VerifyData(recoveredData, 0, 3);
        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData(4);
        secondary->VerifyData(dataToVerify, 4);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary that has made false progress
    // (in terms of sequence number).
    // Pre-existing data on the secondary should be overwritten.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest2)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest2({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that is more advanced
        // than primary.
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->VerifyProgress(7);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (secondary should overwrite all pre-existing data)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database should be identical.
        // Original data on secondary should have been overwritten.
        //
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary that has made false progress.
    // (in terms of epoch)
    // Pre-existing data on the secondary should be overwritten.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest3)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest3({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(5, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that is more advanced
        // than primary.
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(4, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->VerifyProgress(1);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (secondary should overwrite all pre-existing data)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database should be identical.
        // Original data on secondary should have been overwritten.
        //
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary that is already up to date.
    // Data on secondary should remain unchanged.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest4)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest4({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that is more advanced
        // than primary.
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 3, 2, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 4, 3, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 5, 4, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 6, 5, recoveredTypePrefix);
        secondary->DeleteData(typeCount, keyCount, 3, recoveredTypePrefix);
        secondary->VerifyProgress(6);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (secondary should copy no additional data from primary)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Secondary database should remain unchanged.
        //
        secondary->VerifyData(recoveredData);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary that has fallen too far behind.
    // (i.e. tombstones have already been cleaned up on primary)
    // Data on secondary should be overwritten.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest5)
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 500;

        int typeCount = 10;
        int keyCount = 10;
        int extraTypeCount = 10;
        int extraKeyCount = (StoreConfig::GetConfig().TombstoneCleanupLimit / extraTypeCount) + 1;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest5({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(extraTypeCount, extraKeyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(extraTypeCount, extraKeyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(extraTypeCount, extraKeyCount, 1, extraTypePrefix);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Wait for tombstone cleanup to post in threadpool
        //
        Sleep(2000);

        // Populate secondary with different data that is less advanced 
        // than the sequence number of the cleaned up tombstones.
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 6, 5, recoveredTypePrefix);
        secondary->VerifyProgress(2);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (secondary should overwrite all pre-existing data)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Original data on secondary should have been overwritten.
        //
        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial copy data from a primary to a non-empty secondary that has not fallen too far behind
    // even though tombstones have been cleaned up on primary.
    // Pre-existing data on secondary should be preserved.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest6)
    {
        int typeCount = 10;
        int keyCount = 10;
        int extraTypeCount = 10;
        int extraKeyCount = (StoreConfig::GetConfig().TombstoneCleanupLimit / extraTypeCount) + 1;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest6({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(extraTypeCount, extraKeyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(extraTypeCount, extraKeyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(extraTypeCount, extraKeyCount, 1, extraTypePrefix);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that has not fallen behind
        // the tombstone cleanup sequence number on primary
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 6, 5, recoveredTypePrefix);
        secondary->VerifyProgress(3);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (secondary should overwrite all pre-existing data)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Original data on secondary should be preserved
        //
        secondary->VerifyData(recoveredData, 0, 3);
        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData(4);
        secondary->VerifyData(dataToVerify, 4);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Continuation of partial copy data from several different primaries to a non-empty secondary.
    // The secondary should update its "current" epoch each time and avoid full copying.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest7)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix1(L"ExtraType1_");
        wstring extraTypePrefix2(L"ExtraType2_");
        wstring extraTypePrefix3(L"ExtraType3_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest7({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary 1
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix1);
        for (int ix = 1; ix < 20; ++ix)
        {
            primary->UpdateData(typeCount, keyCount, bytesCount * 2, ix, extraTypePrefix1);
        }
        primary->VerifyProgress(20);
        primary->UpdateEpoch(2, 0, 5);
        auto primaryDataToVerify1 = primary->ReadUserData(3, 7); 

        // Populate secondary with different data
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, recoveredTypePrefix);
        secondary->VerifyProgress(2);
        auto recoveredData = secondary->ReadUserData();

        // partial copy from primary 1 to secondary
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7);
        replicator->DrainAndClose();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->VerifyProgress(7);

        // Populate primary 2
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        primary->Close();
        primary.reset();
        DeleteDirectory(DatabasePrimaryDirectory);
        Directory::Create(DatabasePrimaryDirectory);
        primary = PrimaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix2);
        for (int ix = 1; ix < 20; ++ix)
        {
            primary->UpdateData(typeCount, keyCount, bytesCount * 2, ix, extraTypePrefix2);
        }
        primary->VerifyProgress(20);
        primary->UpdateEpoch(2, 0, 5);
        primary->UpdateEpoch(3, 0, 10);
        auto primaryDataToVerify2 = primary->ReadUserData(8, 12); 

        // partial copy from primary 2 to secondary
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 12);
        replicator->DrainAndClose();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->VerifyProgress(12);

        
        // Populate primary 3
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        primary->Close();
        primary.reset();
        DeleteDirectory(DatabasePrimaryDirectory);
        Directory::Create(DatabasePrimaryDirectory);
        primary = PrimaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix3);
        for (int ix = 1; ix < 20; ++ix)
        {
            primary->UpdateData(typeCount, keyCount, bytesCount * 2, ix, extraTypePrefix3);
        }
        primary->VerifyProgress(20);
        primary->UpdateEpoch(2, 0, 5);
        primary->UpdateEpoch(3, 0, 10);
        primary->UpdateEpoch(4, 0, 15);
        auto primaryDataToVerify3 = primary->ReadUserData(13, 17); 

        // partial copy from primary 3 to secondary
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 17);
        replicator->DrainAndClose();

        // Original data on secondary should be preserved
        // and partial copy should have continued from each new
        // primary
        //
        secondary->VerifyData(recoveredData, 0, 2);
        secondary->SetCopyNotificationComplete();

        secondary->VerifyData(primaryDataToVerify1, 3, 7);
        secondary->SetCopyNotificationComplete();

        secondary->VerifyData(primaryDataToVerify2, 8, 12);
        secondary->SetCopyNotificationComplete();

        secondary->VerifyData(primaryDataToVerify3, 13, 17);
        secondary->VerifyProgress(17);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Partial build is performed on a separate database (and replayed on copy completion)
    // when any paging of copy operations occurs. This is needed to ensure
    // transactional consistency.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest8)
    {
        int typeCount = 1;
        int keyCount = 10;
        int bytesCount = 128;

        StoreConfig::GetConfig().TargetCopyOperationSize = bytesCount;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest8({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        // Full copy to secondary for initialization
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0, ErrorCodeValue::Success, false);
        replicator->QueueLastCopyOperation();
        replicator->DrainCopy();

        secondary->VerifyProgress(0);

        // Initial full copy occurred in multiple pages, incrementing the primary's LSN.
        // Reset LSNs for predictable verification.
        //
        replicator->ResetOperationLSNs();

        // Secondary at LSN 0 will be full copied. Seed initial
        // data so that subsequent copies are partial.
        //
        primary->InsertData(typeCount, 1, 1, 1, L"TypeInit");

        replicator->DrainAndClose();
        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);
        
        replicator->ResetQueue();

        // First two operations fit in a single copy page
        //
        primary->InsertData(typeCount, 1, 1, 1, L"Type0A");
        primary->InsertData(typeCount, 1, 1, 1, L"Type0B");

        // Remaining operations will need to be paged
        //
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1A");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1B");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1C");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1D");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1E");

        // These updates create atomic inconsistencies
        //
        primary->DeleteData(typeCount, keyCount, 2, L"Type1C");
        primary->VerifyProgress(9);

        primary->UpdateData(typeCount, keyCount, bytesCount, 2, L"Type1D");
        primary->VerifyProgress(10);

        // Re-open secondary for partial copies
        //
        secondary->Close();
        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Partial copy to secondary.
        // Copy will occur in-place since the data fits in a single (default) copy page.
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Paging occurs - incremental copy must proceed on a snapshot database
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 5, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Snapshot database is not swapped until the overall build completes
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Complete the copy stream, swapping the snapshot database
        //
        replicator->QueueLastCopyOperation();
        replicator->DrainAndClose();
        secondary->VerifyProgress(10);

        // Verify data
        //
        auto primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // An incomplete partial copy database will be deleted on replica open
    // and not applied.
    //
    BOOST_AUTO_TEST_CASE(PartialCopyTest9)
    {
        int typeCount = 1;
        int keyCount = 10;
        int bytesCount = 128;

        StoreConfig::GetConfig().TargetCopyOperationSize = bytesCount;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest9({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        // Full copy to secondary for initialization
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0, ErrorCodeValue::Success, false);
        replicator->QueueLastCopyOperation();
        replicator->DrainCopy();

        secondary->VerifyProgress(0);

        // Initial full copy occurred in multiple pages, incrementing the primary's LSN.
        // Reset LSNs for predictable verification.
        //
        replicator->ResetOperationLSNs();

        // Secondary at LSN 0 will be full copied. Seed initial
        // data so that subsequent copies are partial.
        //
        primary->InsertData(typeCount, 1, 1, 1, L"TypeInit");

        replicator->DrainAndClose();
        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);
        
        replicator->ResetQueue();

        // First two operations fit in a single copy page
        //
        primary->InsertData(typeCount, 1, 1, 1, L"Type0A");
        primary->InsertData(typeCount, 1, 1, 1, L"Type0B");

        // Remaining operations will need to be paged
        //
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1A");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1B");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1C");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1D");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1E");

        // These updates create atomic inconsistencies
        //
        primary->DeleteData(typeCount, keyCount, 2, L"Type1C");
        primary->VerifyProgress(9);

        primary->UpdateData(typeCount, keyCount, bytesCount, 2, L"Type1D");
        primary->VerifyProgress(10);

        // Re-open secondary for partial copies
        //
        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Partial copy to secondary.
        // Copy will occur in-place since the data fits in a single (default) copy page.
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Paging occurs - incremental copy must proceed on a snapshot database
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 5, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Snapshot database is not swapped until the overall build completes
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Partial copy DB is incomplete - it will be deleted and not applied
        //
        while (!secondary->Store.Test_GetCopyStreamComplete())
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Waiting for copy stream complete flag to be set...");
            Sleep(500);
        }
        secondary->Store.Test_SetCopyStreamComplete(false);

        replicator->Close();
        secondary->Close();

        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        secondary->VerifyProgress(3);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        secondary->Close();
        primary->Close();
    }

    // A complete partial copy database will be applied on replica open.
    //
    BOOST_AUTO_TEST_CASE(PartialCopyTest10)
    {
        int typeCount = 1;
        int keyCount = 10;
        int bytesCount = 128;

        StoreConfig::GetConfig().TargetCopyOperationSize = bytesCount;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest10({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        // Full copy to secondary for initialization
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0, ErrorCodeValue::Success, false);
        replicator->QueueLastCopyOperation();
        replicator->DrainCopy();

        secondary->VerifyProgress(0);

        // Initial full copy occurred in multiple pages, incrementing the primary's LSN.
        // Reset LSNs for predictable verification.
        //
        replicator->ResetOperationLSNs();

        // Secondary at LSN 0 will be full copied. Seed initial
        // data so that subsequent copies are partial.
        //
        primary->InsertData(typeCount, 1, 1, 1, L"TypeInit");

        replicator->DrainAndClose();
        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);
        
        replicator->ResetQueue();

        // First two operations fit in a single copy page
        //
        primary->InsertData(typeCount, 1, 1, 1, L"Type0A");
        primary->InsertData(typeCount, 1, 1, 1, L"Type0B");

        // Remaining operations will need to be paged
        //
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1A");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1B");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1C");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1D");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1E");

        // These updates create atomic inconsistencies
        //
        primary->DeleteData(typeCount, keyCount, 2, L"Type1C");
        primary->VerifyProgress(9);

        primary->UpdateData(typeCount, keyCount, bytesCount, 2, L"Type1D");
        primary->VerifyProgress(10);

        // Re-open secondary for partial copies
        //
        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Partial copy to secondary.
        // Copy will occur in-place since the data fits in a single (default) copy page.
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Paging occurs - incremental copy must proceed on a snapshot database
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 5, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Snapshot database is not swapped until the overall build completes
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10, ErrorCodeValue::Success, false);
        replicator->DrainCopy();
        secondary->VerifyProgress(3);

        // Partial copy DB is complete - it will be applied on replica open
        //
        while (!secondary->Store.Test_GetCopyStreamComplete())
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Waiting for copy stream complete flag to be set...");
            Sleep(500);
        }
        secondary->Store.MarkPartialCopyLocalStoreComplete();
        secondary->Store.Test_SetCopyStreamComplete(false);

        replicator->Close();
        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        secondary->VerifyProgress(10);

        // Verify data
        //
        secondary->Store.Test_SetCopyStreamComplete(true);
        replicator->QueueLastCopyOperation();
        replicator->DrainAndClose();

        auto primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        secondary->Close();
        primary->Close();
    }

    // Partial copy cannot be performed if the configuration number does not match.
    //
    BOOST_AUTO_TEST_CASE(PartialCopyTest11)
    {
        int typeCount = 1;
        int keyCount = 10;
        int bytesCount = 128;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest11({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 42, 0);

        // Full copy to secondary for initialization
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0, ErrorCodeValue::Success, false);
        replicator->QueueLastCopyOperation();
        replicator->DrainCopy();

        secondary->VerifyProgress(0);

        primary->VerifyCopyCounts(1, 0);

        // Initial full copy occurred in multiple pages, incrementing the primary's LSN.
        // Reset LSNs for predictable verification.
        //
        replicator->ResetOperationLSNs();

        // Secondary at LSN 0 will be full copied.
        //
        primary->InsertData(typeCount, 1, 1, 1, L"TypeInit");

        replicator->DrainAndClose();
        primary->VerifyProgress(1);
        secondary->VerifyProgress(1);
        
        replicator->ResetQueue();

        // Add data for false progress detection
        //
        primary->UpdateEpoch(3, 42, 1);
        primary->UpdateEpoch(5, 42, 1);
        primary->UpdateEpoch(7, 42, 1);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1A");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1B");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1C");
        primary->UpdateEpoch(9, 42, 4);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1D");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1E");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, L"Type1F");
        primary->UpdateEpoch(13, 42, 7);

        // Re-open secondary for false progress full copy
        //
        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        // Put secondary in an epoch where the LSN and dataloss number matches,
        // but configuration number does not.
        //
        secondary->UpdateEpoch(2, 42, 1);

        // Full copy to secondary due to false progress
        //
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7, ErrorCodeValue::Success, false);
        replicator->DrainAndClose();

        primary->VerifyProgress(7);
        secondary->VerifyProgress(7);

        primary->VerifyCopyCounts(2, 0);

        // Verify data
        //
        auto primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        secondary->Close();
        primary->Close();
    }

    // Fault secondary apply operation to trigger retry path during apply.
    // 
    BOOST_AUTO_TEST_CASE(PartialCopyTest12)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        wstring extraTypePrefix(L"ExtraType_");
        wstring recoveredTypePrefix(L"RecoveredType_");

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PartialCopyTest12({0}, {1}, {2})", typeCount, keyCount, bytesCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2);
        primary->DeleteData(typeCount, keyCount, 3);
        primary->InsertData(typeCount, keyCount, bytesCount, 1, extraTypePrefix);
        primary->UpdateData(typeCount, keyCount, bytesCount * 2, 2, extraTypePrefix);
        primary->DeleteData(typeCount, keyCount, 3, extraTypePrefix);
        primary->VerifyProgress(6);
        primary->UpdateEpoch(2, 0, 6);
        primary->UpdateEpoch(3, 0, 6);
        primary->UpdateEpoch(4, 0, 6);
        ::FABRIC_SEQUENCE_NUMBER copyOperationLSN = replicator->LastOperationLSN;

        // Populate secondary with different data that is less advanced
        // than primary.
        //
        replicator->ResetQueue();
        replicator->ResetOperationLSNs();
        secondary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        secondary->UpdateEpoch(1, 0, 0);
        secondary->InsertData(typeCount, keyCount, bytesCount * 2, 1, recoveredTypePrefix);
        secondary->UpdateData(typeCount, keyCount, bytesCount * 4, 2, recoveredTypePrefix);
        secondary->DeleteData(typeCount, keyCount, 3, recoveredTypePrefix);
        secondary->VerifyProgress(3);

        // Verify data is different on primary and secondary
        //
        auto primaryData = primary->ReadUserData();
        auto recoveredData = secondary->ReadUserData();
        VERIFY(primaryData.size() != recoveredData.size(), "primary = {0} secondary = {1}", primaryData.size(), recoveredData.size());

        // partial copy from primary to secondary
        // (should not overwrite any data already on secondary)
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

        secondary->Store.Test_SetSecondaryApplyFaultInjectionEnabled(true);
        FaultInjector::GetGlobalFaultInjector().InjectTransientError(FaultInjector::ReplicatedStoreApplyTag, ErrorCodeValue::StoreUnexpectedError);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, copyOperationLSN);
        replicator->DrainAndClose();

        // Primary and secondary database should NOT be identical.
        // Original data on secondary should be preserved and more
        // advanced primary data should have been copied.
        //
        secondary->VerifyData(recoveredData, 0, 3);
        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData(4);
        secondary->VerifyData(dataToVerify, 4);
        secondary->VerifyProgress(6);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

#if !defined(PLATFORM_UNIX)
    // Tests configuration setting overrides for different full copy modes
    //
    BOOST_AUTO_TEST_CASE(CopyModeTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** CopyModeTest");

        ReplicatedStoreSettings settings;
        
        // Default setting on replica causes full copy mode to read from config
        {
            StoreConfig::GetConfig().EnableFileStreamFullCopy = true;
            auto replicator = make_com<ComMockReplicator>();
            auto primary = PrimaryReplica::Create(replicator);
            auto secondary = SecondaryReplica::Create(replicator);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            primary->InsertData(1, 1, 1);
            primary->VerifyProgress(1);

            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            secondary->UpdateEpoch(1, 0, 0);
            replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
            replicator->DrainAndClose();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(1);

            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 0); 

            primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

            primary->Close();
            secondary->Close();
        }

        // Default setting on replica causes full copy mode to read from config
        {
            StoreConfig::GetConfig().EnableFileStreamFullCopy = false;
            auto replicator = make_com<ComMockReplicator>();
            auto primary = PrimaryReplica::Create(replicator);
            auto secondary = SecondaryReplica::Create(replicator);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            primary->InsertData(1, 1, 1);
            primary->VerifyProgress(1);

            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
            replicator->DrainAndClose();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(1);

            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 0); 

            primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

            primary->Close();
            secondary->Close();
        }

        // ReplicatedStoreSettings override config settings
        {
            StoreConfig::GetConfig().EnableFileStreamFullCopy = false;
            auto replicator = make_com<ComMockReplicator>();
            auto primary = PrimaryReplica::Create(replicator);
            auto secondary = SecondaryReplica::Create(replicator);

            settings.FullCopyMode = FullCopyMode::Physical;
            primary->UpdateSettings(settings);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            primary->InsertData(1, 1, 1);
            primary->VerifyProgress(1);

            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
            replicator->DrainAndClose();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(1);

            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 0); 

            primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

            primary->Close();
            secondary->Close();
        }
        
        // ReplicatedStoreSettings override config settings
        {
            StoreConfig::GetConfig().EnableFileStreamFullCopy = true;
            auto replicator = make_com<ComMockReplicator>();
            auto primary = PrimaryReplica::Create(replicator);
            auto secondary = SecondaryReplica::Create(replicator);

            settings.FullCopyMode = FullCopyMode::Logical;
            primary->UpdateSettings(settings);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            primary->InsertData(1, 1, 1);
            primary->VerifyProgress(1);

            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
            replicator->DrainAndClose();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(1);

            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 0); 

            primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

            primary->Close();
            secondary->Close();
        }

        // ReplicatedStoreSettings override config settings
        {
            StoreConfig::GetConfig().EnableFileStreamFullCopy = false;
            auto replicator = make_com<ComMockReplicator>();
            auto primary = PrimaryReplica::Create(replicator);
            auto secondary = SecondaryReplica::Create(replicator);

            settings.FullCopyMode = FullCopyMode::Rebuild;
            primary->UpdateSettings(settings);

            primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
            primary->UpdateEpoch(1, 0, 0);
            primary->InsertData(10, 10, 1);
            primary->UpdateData(10, 10, 1, 2);
            primary->DeleteData(10, 10, 3);
            primary->VerifyProgress(3);

            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
            replicator->DrainAndClose();

            auto dataToVerify = primary->ReadUserData();
            secondary->VerifyData(dataToVerify);
            secondary->VerifyProgress(3);

            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 0); 
            VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 1); 

            primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);
            secondary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

            primary->Close();
            secondary->Close();
        }
    }

    // Tests that the progress vector history is truncated by a secondary replica
    // after build if the history contains LSNs that are greater than the secondary
    // replica's actual progress.
    //
    // This can happen if a primary performs a build soon after receiving an epoch update
    // but before the relevant LSNs in the epoch update have been quorum-acked.
    //
    BOOST_AUTO_TEST_CASE(ProgressVectorTruncationTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ProgressVectorTruncationTest");

        auto replicator = make_com<ComMockReplicator>();
        auto primary = PrimaryReplica::Create(replicator);
        auto secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(1, 1, 1, 1, L"Type1_");

        // Initialize secondary to prevent full copies
        //
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);

        auto operationLSN = replicator->LastOperationLSN;
        replicator->QueueCopyOperations(primary->Store, secondary->Store, operationLSN);
        replicator->DrainAndClose();
        secondary->Close();

        // Insert rest of primary data
        //
        replicator->ResetQueue();
        operationLSN = replicator->LastOperationLSN;

        primary->UpdateEpoch(2, 0, 1);
        primary->InsertData(1, 1, 1, 1, L"Type2_");
        primary->UpdateEpoch(3, 0, operationLSN + 1);
        primary->InsertData(1, 1, 1, 1, L"Type3_");
        primary->UpdateEpoch(4, 0, operationLSN + 2);
        primary->InsertData(1, 1, 1, 1, L"Type4_");
        primary->UpdateEpoch(5, 0, operationLSN + 3);
        primary->InsertData(1, 1, 1, 1, L"Type5_");
        primary->UpdateEpoch(6, 0, operationLSN + 4);

        auto primaryProgress = replicator->LastOperationLSN;

        // Truncate progress vector entries based on upToLsn
        //
        for (auto targetLsn = operationLSN - 1; targetLsn <= primaryProgress; ++targetLsn)
        {
            secondary->Close();
            replicator->ResetQueue();
            secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);

            replicator->QueueCopyOperations(primary->Store, secondary->Store, targetLsn);
            replicator->DrainAndClose();

            auto progressVectorToVerify = ProgressVectorData(primary->ReadProgressVector(false));
            auto isTruncated = progressVectorToVerify.TryTruncate(targetLsn);

            if (targetLsn < primaryProgress)
            {
                VERIFY_IS_TRUE(isTruncated);
            }
            else
            {
                VERIFY_IS_FALSE(isTruncated);
            }

            secondary->VerifyProgressVector(progressVectorToVerify.ProgressVector, false);
        }

        secondary->Close();
        primary->Close();
    }

    // Test replay path for FullCopyMode::Rebuild
    //
    BOOST_AUTO_TEST_CASE(FileStreamRebuildCopyTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** FileStreamRebuildCopyTest");

        StoreConfig::GetConfig().DatabaseRebuildBatchSizeInBytes = 1024;
             
        auto replicator = make_com<ComMockReplicator>();
        auto primary = PrimaryReplica::Create(replicator);
        auto secondary = SecondaryReplica::Create(replicator);

        ReplicatedStoreSettings settings;
        settings.FullCopyMode = FullCopyMode::Rebuild;
        primary->UpdateSettings(settings);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(10, 10, 128);
        primary->UpdateData(10, 10, 512, 2);
        primary->DeleteData(10, 10, 3);

        primary->UpdateEpoch(2, 0, 3);
        primary->InsertData(5, 5, 1024, 1, L"Type2_");
        primary->InsertData(5, 10, 1024, 1, L"Type3_");
        primary->DeleteData(5, 10, 2, L"Type3_");
        primary->VerifyProgress(6);

        primary->UpdateEpoch(3, 0, 6);
        primary->InsertData(1, 10, 128, 1, L"__default__");
        primary->VerifyProgress(7);

        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7);
        replicator->DrainAndClose();

        VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetPartialCopyCount(), 0); 
        VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFullCopyCount(), 1); 
        VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamFullCopyCount(), 0); 
        VERIFY_ARE_EQUAL(primary->Store.CopyStatistics.GetFileStreamRebuildFullCopyCount(), 1); 

        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        secondary->VerifyProgress(7);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        primary->Close();
        secondary->Close();
    }
#endif

    // Test that closing the secondary will short-circuit and abort any outstanding
    // copy notification processing
    //
    BOOST_AUTO_TEST_CASE(CopyNotificationAbortTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** CopyNotificationAbortTest");

        auto replicator = make_com<ComMockReplicator>();
        auto primary = PrimaryReplica::Create(replicator);
        auto secondary = SecondaryReplica::Create(replicator);

        // Populate primary
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        primary->InsertData(2, 2, 2, 1, L"Type1");
        primary->InsertData(2, 2, 2, 1, L"Type2");
        primary->InsertData(2, 2, 2, 1, L"Type3");
        primary->InsertData(2, 2, 2, 1, L"Type4");
        primary->InsertData(2, 2, 2, 1, L"Type5");
        primary->InsertData(2, 2, 2, 1, L"Type6");
        primary->VerifyProgress(6);

        // copy everything from primary to blank secondary
        //
        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        secondary->SetCopyNotificationDelay(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds / 2));

        ::FABRIC_SEQUENCE_NUMBER operationLSN = replicator->LastOperationLSN;
        replicator->QueueCopyOperations(primary->Store, secondary->Store, operationLSN, ErrorCodeValue::Success, false);
        replicator->Drain();

        AutoResetEvent waiter(false);
        auto operation = secondary->Store.BeginClose(
            [this, &waiter](AsyncOperationSPtr const &) { waiter.Set(); },
            secondary->CreateAsyncOperationRoot());

        replicator->QueueLastCopyOperation();
        replicator->Close();
        
        // Close should abort the pending copy notification
        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

        auto error = secondary->Store.EndClose(operation);
        VERIFY_IS_TRUE(error.IsSuccess());

        primary->Close();
    }

    // Operations should be blocked if the replica is not an open primary
    //
    BOOST_AUTO_TEST_CASE(ErrorContractTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** ErrorContractTest");

        ComPointer<IFabricReplicator> fabricReplicator;
        auto replicator = make_com<ComMockReplicator>();
        replicator->QueryInterface(IID_IFabricReplicator, fabricReplicator.VoidInitializationAddress());

        wstring type(L"Type");
        wstring key(L"Key");
        ErrorCode error;
        TransactionSPtr txSPtr;
        EnumerationSPtr enumSPtr;

        // Cannot create valid transactions before opening the store
        //
        PrimaryReplicaSPtr unopenedReplica = PrimaryReplica::Create(replicator);
        auto unopenedStore = EseReplicatedStore::Create(
            Guid::NewGuid(), 
            0, 
            move(fabricReplicator),
            ReplicatedStoreSettings(),
            EseLocalStoreSettings(
                DatabaseFilename, 
                DatabasePrimaryDirectory),
            Api::IStoreEventHandlerPtr(),
            Api::ISecondaryEventHandlerPtr(),
            *unopenedReplica);
        unopenedReplica->SetStore(std::move(unopenedStore));
        error = unopenedReplica->Store.CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));
        unopenedReplica.reset();

        // Cannnot create valid transactions unless the store is in the primary role
        //
        PrimaryReplicaSPtr replica = PrimaryReplica::Create(replicator);
        replica->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        error = replica->Store.CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

        // Cannot create valid transactions after closing the store
        //
        replicator->Close();
        error = replica->Store.CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));
        replica->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        replica->Close();
        error = replica->Store.CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));
    }

    // Progress vector and other metadata should not count towards user data progress
    //
    BOOST_AUTO_TEST_CASE(MetadataTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** MetadataTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);

        // Empty replica has a progress of 0
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->VerifyProgress(0);

        // Metadata does not count towards user data progress
        primary->UpdateEpoch(1, 0, 0);
        primary->VerifyProgress(0);

        // Now insert user data
        primary->InsertData(1, 1, 1);
        primary->VerifyProgress(1);

        // Metadata
        primary->UpdateEpoch(2, 0, 0);
        primary->VerifyProgress(1);

        primary->UpdateEpoch(3, 0, 0);
        primary->VerifyProgress(1);

        // Insert more user data
        primary->InsertData(10, 10, 1, 1, L"MoreData_");
        primary->VerifyProgress(2);

        // Metadata
        primary->UpdateEpoch(4, 0, 0);
        primary->VerifyProgress(2);

        vector<byte> bytes;

        // Cannot perform write operations on metadata types

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, Constants::ProgressDataType, L"Key", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Update(txSPtr, Constants::ProgressDataType, L"Key", 0, L"NewKey", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(txSPtr, Constants::ProgressDataType, L"Key", 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, Constants::TombstoneDataType, L"Key", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Update(txSPtr, Constants::TombstoneDataType, L"Key", 0, L"NewKey", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(txSPtr, Constants::TombstoneDataType, L"Key", 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }
                
        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, Constants::FabricTimeDataType, L"Key", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Update(txSPtr, Constants::FabricTimeDataType, L"Key", 0, L"NewKey", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(txSPtr, Constants::FabricTimeDataType, L"Key", 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, Constants::LocalStoreIncrementalBackupDataType, L"Key", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Update(txSPtr, Constants::LocalStoreIncrementalBackupDataType, L"Key", 0, L"NewKey", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(txSPtr, Constants::LocalStoreIncrementalBackupDataType, L"Key", 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, Constants::PartialCopyProgressDataType, L"Key", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Update(txSPtr, Constants::PartialCopyProgressDataType, L"Key", 0, L"NewKey", bytes.data(), 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        {
            TransactionSPtr txSPtr;
            ErrorCode error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Delete(txSPtr, Constants::PartialCopyProgressDataType, L"Key", 0);
            VERIFY_ARE_EQUAL(error.ToHResult(), E_INVALIDARG);
        }

        replicator->ResetQueue();
        replicator->Close();
        primary->Close();
    }

    // Calls to ChangeRole() and Close() should block while there are pending active transactions or an active replicator
    // (only if transaction draining is disabled).
    // 
    BOOST_AUTO_TEST_CASE(PendingStateChangeTest)
    {
        StoreConfig::GetConfig().TransactionDrainTimeout = TimeSpan::Zero;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PendingStateChangeTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr replica = PrimaryReplica::Create(replicator);
        replica->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        replica->UpdateEpoch(1, 0, 0);

        // ChangeRole() - primary->secondary, single active transaction
        //
        {
            bool txActive = true;
            
            TransactionSPtr txSPtr;
            ErrorCode error = replica->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            AutoResetEvent waiter(false);

            auto operation = replica->Store.BeginChangeRole(
                ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                [&waiter, &txActive](AsyncOperationSPtr const &)
                {
                    VERIFY_IS_TRUE( ! txActive);
                    waiter.Set();
                },
                replica->CreateAsyncOperationRoot());

            Sleep(1000);
            txActive = false;
            txSPtr.reset();
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            wstring unused;
            error = replica->Store.EndChangeRole(operation, unused);
            VERIFY_IS_TRUE(error.IsSuccess(), L"EndChangeRole()");
        }

        // ChangeRole() - secondary->primary
        //
        {
            bool replicatorActive = true;
            AutoResetEvent waiter(false);

            auto operation = replica->Store.BeginChangeRole(
                ::FABRIC_REPLICA_ROLE_PRIMARY,
                [&waiter, &replicatorActive](AsyncOperationSPtr const &)
                {
                    VERIFY_IS_TRUE( ! replicatorActive);
                    waiter.Set();
                },
                replica->CreateAsyncOperationRoot());

            Sleep(1000);
            replicatorActive = false;
            replicator->Close();
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            wstring unused;
            ErrorCode error = replica->Store.EndChangeRole(operation, unused);
            VERIFY_IS_TRUE(error.IsSuccess(), L"EndChangeRole()");
        }

        // ChangeRole() - primary->secondary, multiple active transactions
        //
        replicator->ResetQueue();
        {
            bool txActive = true;
            bool tx2Active = true;
            TransactionSPtr txSPtr;
            TransactionSPtr tx2SPtr;

            ErrorCode error = replica->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = replica->Store.CreateTransaction(tx2SPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            AutoResetEvent waiter(false);

            auto operation = replica->Store.BeginChangeRole(
                ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
                [&waiter, &txActive, &tx2Active](AsyncOperationSPtr const &)
                {
                    VERIFY_IS_TRUE( ! txActive);
                    VERIFY_IS_TRUE( ! tx2Active);
                    waiter.Set();
                },
                replica->CreateAsyncOperationRoot());

            Sleep(1000);
            txActive = false;
            txSPtr.reset();
            Sleep(1000);
            tx2Active = false;
            tx2SPtr.reset();
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            wstring unused;
            error = replica->Store.EndChangeRole(operation, unused);
            VERIFY_IS_TRUE(error.IsSuccess(), L"EndChangeRole()");
        }

        // ChangeRole() - secondary->primary, outstanding invalid transactions should not block
        {
            TransactionSPtr txInvalidSPtr;
            ErrorCode error = replica->Store.CreateTransaction(txInvalidSPtr);
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::NotPrimary));

            replicator->Close();

            replica->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        }

        // Close() - secondary role
        //
        replicator->ResetQueue();
        replica->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        {
            bool replicatorActive = true;
            AutoResetEvent waiter(false);

            auto operation = replica->Store.BeginClose(
                [&waiter, &replicatorActive](AsyncOperationSPtr const &)
                {
                    VERIFY_IS_TRUE( ! replicatorActive);
                    waiter.Set();
                },
                replica->CreateAsyncOperationRoot());

            Sleep(1000);
            replicatorActive = false;
            replicator->Close();
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            ErrorCode error = replica->Store.EndClose(operation);
            VERIFY_IS_TRUE(error.IsSuccess(), L"EndClose()");
        }

        // Close() - primary role, multiple active transactions should block
        //
        //
        replica = PrimaryReplica::Create(replicator);
        replica->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        {
            TransactionSPtr txSPtr;
            TransactionSPtr tx2SPtr;

            ErrorCode error = replica->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = replica->Store.CreateTransaction(tx2SPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            bool txActive = true;
            bool tx2Active = true;

            AutoResetEvent waiter(false);

            auto operation = replica->Store.BeginClose(
                [&waiter, &txActive, &tx2Active](AsyncOperationSPtr const &)
                {
                    VERIFY_IS_TRUE( ! txActive);
                    VERIFY_IS_TRUE( ! tx2Active);
                    waiter.Set();
                },
                replica->CreateAsyncOperationRoot());

            Sleep(1000);
            txActive = false;
            txSPtr.reset();
            Sleep(1000);
            tx2Active = false;
            tx2SPtr.reset();
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            error = replica->Store.EndClose(operation);
            VERIFY_IS_TRUE(error.IsSuccess(), L"EndClose()");
        }
    }

    BOOST_AUTO_TEST_CASE(TombstoneParsingTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneParsingTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);

        TombstoneParsingHelper(primary->Store, L"MyType", L"MyKey");

        TombstoneParsingHelper(primary->Store, L"+MyType", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"My+Type", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType+", L"MyKey");

        TombstoneParsingHelper(primary->Store, L"MyType", L"+MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType", L"My+Key");
        TombstoneParsingHelper(primary->Store, L"MyType", L"MyKey+");

        TombstoneParsingHelper(primary->Store, L"++MyType", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"My++Type", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType++", L"MyKey");

        TombstoneParsingHelper(primary->Store, L"MyType", L"++MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType", L"My++Key");
        TombstoneParsingHelper(primary->Store, L"MyType", L"MyKey++");

        TombstoneParsingHelper(primary->Store, L"MyType+", L"+MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType++", L"++MyKey");
        TombstoneParsingHelper(primary->Store, L"M+y+T+y+p+e", L"+M+y+K+e+y+");

        TombstoneParsingHelper(primary->Store, L"(+)MyType", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"My(+)Type", L"MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType(+)", L"MyKey");

        TombstoneParsingHelper(primary->Store, L"MyType", L"(+)MyKey");
        TombstoneParsingHelper(primary->Store, L"MyType", L"My(+)Key");
        TombstoneParsingHelper(primary->Store, L"MyType", L"MyKey(+)");

        TombstoneParsingHelper(primary->Store, L"MyType(+)", L"(+)MyKey");

        replicator->Close();
        primary->Close();
    }

    // Trigger tombstone cleanup with a single delete operation containing many deletes.
    //
    void TombstoneCleanupTest1();

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest1_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneCleanupTest1();
    }

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest1_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneCleanupTest1();
    }

    void TombstoneCleanupTest1()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;

        int typeCount = 10;
        int keyCount = StoreConfig::GetConfig().TombstoneCleanupLimit / typeCount;

        int extraTypeCount = 2;
        int totalDataCount = (typeCount + extraTypeCount) * keyCount;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneCleanupTest1({0}, {1}, {2}, cleanup2={3})", typeCount, keyCount, extraTypeCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary and secondary with data
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();
        primary->InsertData(typeCount + extraTypeCount, keyCount, 1);
        replicator->DrainAndClose();

        auto primaryData = primary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(totalDataCount));

        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == static_cast<size_t>(totalDataCount));

        // Deleting sufficient data should trigger tombstone cleanup
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();
        primary->DeleteData(typeCount + (extraTypeCount / 2), keyCount, 1);
        replicator->DrainAndClose();

        size_t deletedCount = (typeCount + (extraTypeCount / 2)) * keyCount;
        size_t expectedRemaining = totalDataCount - deletedCount / 2;

        // Wait for tombstones to get cleaned up on both replicas
        //
        primaryData = primary->WaitForDataCount(expectedRemaining);
        secondary->WaitForDataCount(expectedRemaining);

        secondary->SetCopyNotificationComplete();

        // Database contents should be identical
        //
        secondary->VerifyData(primaryData);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // The tombstone count is kept in memory and recovered from the database on Open().
    // This tests that the recovery occurs correctly by verifying that tombstone cleanup 
    // occurs correctly after a secondary "crashes".
    //
    // Also mix up some of the type names for the data and use more than one replicated
    // operation to trigger cleanup.
    //
    void TombstoneCleanupTest2();

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest2_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneCleanupTest2();
    }

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest2_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneCleanupTest2();
    }

    void TombstoneCleanupTest2()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;

        int typeCount = 10;
        int keyCount = StoreConfig::GetConfig().TombstoneCleanupLimit / typeCount;

        int extraTypeCount = 3;
        wstring extraTypePrefix(L"ExtraType_");
        int totalDataCount = (typeCount + extraTypeCount) * keyCount;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneCleanupTest2({0}, {1}, {2}, cleanup2={3})", typeCount, keyCount, extraTypeCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary and secondary with data
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        secondary->UpdateEpoch(1, 0, 0);
        replicator->QueueLastCopyOperation();

        primary->InsertData(typeCount, keyCount, 1);
        primary->InsertData(extraTypeCount, keyCount, 1, 1, extraTypePrefix);
        replicator->DrainAndClose();

        auto primaryData = primary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(totalDataCount));

        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == static_cast<size_t>(totalDataCount));

        // Delete some data but don't trigger tombstone cleanup yet
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        primary->DeleteData(typeCount - 1 , keyCount, 1);
        replicator->DrainAndClose();

        // Crash and recover the secondary
        //
        secondary->Close();
        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        replicator->QueueLastCopyOperation();

        // Trigger tombstone cleanup. Secondary should have recovered
        // the tombstone count and should cleanup as well
        //
        primary->DeleteData(extraTypeCount, keyCount, 2, extraTypePrefix);
        replicator->DrainAndClose();

        size_t deletedCount = ((typeCount - 1) * keyCount) + ((extraTypeCount * keyCount) / 2);
        size_t expectedRemaining = totalDataCount - deletedCount / 2;

        // Wait for tombstones to get cleaned up on both replicas
        //
        primaryData = primary->WaitForDataCount(expectedRemaining);
        secondary->WaitForDataCount(expectedRemaining);

        // Database contents should be identical
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        secondary->Store.Test_SetCopyStreamComplete(true);
        replicator->QueueLastCopyOperation();
        replicator->DrainAndClose();

        secondary->VerifyData(primaryData);

        auto lowWatermark = primary->GetTombstoneLowWatermark();
        secondary->VerifyTombstoneLowWatermark(lowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Ensure that partial copy correctly releases the low watermark lock, allowing tombstone
    // cleanup to proceed
    //
    void TombstoneCleanupTest3();

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest3_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneCleanupTest3();
    }

    BOOST_AUTO_TEST_CASE(TombstoneCleanupTest3_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneCleanupTest3();
    }

    void TombstoneCleanupTest3()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;

        int typeCount = 10;
        int keyCount = 20;

        int initialTombstonesTypeCount = 10;
        int initialTombstonesKeyCount = (StoreConfig::GetConfig().TombstoneCleanupLimit - 100) / initialTombstonesTypeCount;

        int extraTypeCount = 10;
        int extraKeyCount = 10;
        int extraTypeFactor = 10;

        int bytesCount = 1024;

        int totalDataCount = 
            (typeCount * keyCount) 
            + (initialTombstonesTypeCount * initialTombstonesKeyCount) 
            + (extraTypeCount * extraKeyCount * extraTypeFactor);

        wstring extraTypePrefix = L"ExtraType_";
        wstring initialTombstonesTypePrefix = L"InitTombstoneType_";

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneCleanupTest3({0}, {1}, {2}, cleanup2={3})", typeCount, keyCount, extraTypeCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary with data and initial tombstones
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        secondary->UpdateEpoch(1, 0, 0);

        // Replicate initial data
        primary->InsertData(typeCount, keyCount, bytesCount);
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

        // The following items will be copied
        for (int ix = 0; ix < extraTypeFactor; ++ix)
        {
            wstring typePrefix;
            StringWriter writer(typePrefix);
            writer.Write("{0}{1}_", extraTypePrefix, ix);

            primary->InsertData(extraTypeCount, extraKeyCount, bytesCount, 1, typePrefix);
        }
        primary->InsertData(initialTombstonesTypeCount, initialTombstonesKeyCount, bytesCount, 1, initialTombstonesTypePrefix);
        primary->DeleteData(initialTombstonesTypeCount, initialTombstonesKeyCount, 1, initialTombstonesTypePrefix);

        primary->VerifyProgress(13);
        secondary->VerifyProgress(1);

        // The first sequence of data items will be deleted to trigger tombstone cleanup,
        // do not include them in verification
        //
        auto primaryData = primary->ReadUserData(2);
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(totalDataCount - (typeCount * keyCount)));

        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == static_cast<size_t>(typeCount * keyCount));

        replicator->ResetQueue();
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 13);

        // Deleting sufficient data should trigger tombstone cleanup
        //
        primary->DeleteData(typeCount, keyCount, 1);

        size_t deletedCount = (initialTombstonesTypeCount * initialTombstonesKeyCount) + (typeCount * keyCount);
        size_t expectedRemaining = totalDataCount - deletedCount / 2;

        // Wait for tombstones to get cleaned up on primary
        //
        primary->WaitForDataCount(expectedRemaining);

        // Drain all copy and replicated operations to secondary
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->DrainAndClose();

        // Wait for tombstones to get cleaned up on secondary
        //
        secondary->WaitForDataCount(expectedRemaining);

        primaryData = primary->ReadUserData();
        secondary->VerifyData(primaryData);

        auto progressVectorToVerify = primary->ReadProgressVector();
        secondary->VerifyProgressVector(progressVectorToVerify);

        auto primaryLowWatermark = primary->GetTombstoneLowWatermark();
        auto secondaryLowWatermark = secondary->GetTombstoneLowWatermark();
        VERIFY(primaryLowWatermark == secondaryLowWatermark, 
            "Low Watermarks: primary={0} secondary={1} expect(primary > secondary)", 
            primaryLowWatermark, 
            secondaryLowWatermark);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Trigger tombstone cleanup on the secondary with many deletes and
    // ensure that no conflicts occur between updating tombstones
    // and cleanup on the secondary
    //
    void TombstoneSecondaryConflictTest();

    BOOST_AUTO_TEST_CASE(TombstoneSecondaryConflictTest_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneSecondaryConflictTest();
    }

    BOOST_AUTO_TEST_CASE(TombstoneSecondaryConflictTest_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneSecondaryConflictTest();
    }

    void TombstoneSecondaryConflictTest()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 20;

        int typeCount = 1;
        int keyCount = StoreConfig::GetConfig().TombstoneCleanupLimit * 2;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneSecondaryConflictTest({0}, {1}, cleanup2={2})", typeCount, keyCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        Random rand;
        LONG workersCount = 5;
        ManualResetEvent workersDoneEvent(false);
        Common::atomic_long pendingWorkers(workersCount);

        // Replicate deletes to secondary
        //
        for (auto ix=0; ix<workersCount; ++ix)
        {
            int index = ix;

            Threadpool::Post([&, index]()
            {
                wstring keyPrefix = wformatString("TestKey-{0}-", index);

                for (auto ix=0; ix<100; ++ix)
                {
                    auto rCount = rand.Next(keyCount + 1);
                    primary->InsertData(typeCount, rCount, 1, 1, L"TestType", keyPrefix);
                    primary->DeleteData(typeCount, rCount, 1, L"TestType", keyPrefix, ErrorCodeValue::StoreWriteConflict);
                }

                if (--pendingWorkers == 0)
                {
                    workersDoneEvent.Set();
                }
            });
        }

        workersDoneEvent.WaitOne();

        primary->VerifyFaultCount();
        secondary->VerifyFaultCount();

        replicator->DrainAndClose();

        primary->VerifyFaultCount();
        secondary->VerifyFaultCount();

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // Test that the tombstone low watermark LSN cannot decrease due to a partial copy
    //
    void TombstoneLowWatermarkTest();

    BOOST_AUTO_TEST_CASE(TombstoneLowWatermarkTest_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneLowWatermarkTest();
    }

    BOOST_AUTO_TEST_CASE(TombstoneLowWatermarkTest_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneLowWatermarkTest();
    }

    void TombstoneLowWatermarkTest()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 99;

        int keyCount = StoreConfig::GetConfig().TombstoneCleanupLimit + 1;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneLowWatermarkTest({0}, cleanup2={1})", keyCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary and secondary with data
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);

        primary->InsertData(1, keyCount, 1, 1, L"LiveType1_");
        primary->InsertData(1, keyCount, 1, 1, L"ToDeleteType1_");
        primary->InsertData(1, keyCount, 1, 1, L"LiveType2_");
        primary->InsertData(1, keyCount, 1, 1, L"ToDeleteType2_");

        replicator->DrainAndClose();

        int totalDataCount = keyCount * 4;

        auto primaryData = primary->ReadUserData();
        VERIFY_IS_TRUE(primaryData.size() == static_cast<size_t>(totalDataCount));

        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);
        auto secondaryData = secondary->ReadUserData();
        VERIFY_IS_TRUE(secondaryData.size() == static_cast<size_t>(totalDataCount));

        // Deleting sufficient data should trigger tombstone cleanup and
        // establish first low watermark
        //
        replicator->ResetQueue();
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();
        primary->DeleteData(1, keyCount, 1, L"ToDeleteType1_");
        replicator->Drain();

        size_t deletedCount = keyCount / 2;
        size_t expectedRemaining = totalDataCount - deletedCount;

        // Wait for tombstones to get cleaned up on both replicas
        //
        primary->WaitForDataCount(expectedRemaining);
        secondary->WaitForDataCount(expectedRemaining);

        auto primaryProgress = primary->ReadProgress();
        auto secondaryProgress = secondary->ReadProgress();
        auto firstTombstoneCleanupProgress = primaryProgress;
        VERIFY(primaryProgress == secondaryProgress, "[1] Progress: primary={0} secondary={1}", primaryProgress, secondaryProgress);

        auto primaryLowWatermark = primary->GetTombstoneLowWatermark();
        auto secondaryLowWatermark = secondary->GetTombstoneLowWatermark();
        VERIFY(primaryLowWatermark == secondaryLowWatermark, 
            "Low Watermarks: primary={0} secondary={1} (equals)", 
            primaryLowWatermark, 
            secondaryLowWatermark);

        // Disable tombstone cleanup on primary so that the low watermark on the
        // primary falls behind the secondary
        //
        primary->Store.Test_SetTombstoneCleanupEnabled(false);
        primary->DeleteData(1, keyCount, 1, L"ToDeleteType2_");
        replicator->DrainAndClose();

        deletedCount += (deletedCount + keyCount) / 2;
        expectedRemaining = totalDataCount - deletedCount;

        secondary->WaitForDataCount(expectedRemaining);

        primaryLowWatermark = primary->GetTombstoneLowWatermark();
        secondaryLowWatermark = secondary->GetTombstoneLowWatermark();
        VERIFY(primaryLowWatermark < secondaryLowWatermark, 
            "[1] Low Watermarks: primary={0} secondary={1} (primary < secondary)", 
            primaryLowWatermark, 
            secondaryLowWatermark);

        primaryProgress = primary->ReadProgress();
        secondaryProgress = secondary->ReadProgress();
        VERIFY(primaryProgress == secondaryProgress, "[2] Progress: primary={0} secondary={1}", primaryProgress, secondaryProgress);

        // Partial build secondary - the low watermark on the secondary is 
        // already higher so it should not be modified
        // 
        secondary->Close();
        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY,
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, primaryProgress);
        replicator->DrainAndClose();

        primaryLowWatermark = primary->GetTombstoneLowWatermark();
        secondaryLowWatermark = secondary->GetTombstoneLowWatermark();
        VERIFY(primaryLowWatermark < secondaryLowWatermark, 
            "[2] Low Watermarks: primary={0} secondary={1} (primary < secondary)", 
            primaryLowWatermark, 
            secondaryLowWatermark);

        // Database contents should be identical except for the tombstones
        // that haven't been cleaned up on primary
        //
        primaryData = primary->ReadUserData(0, firstTombstoneCleanupProgress - 1);
        secondary->VerifyData(primaryData, 0, firstTombstoneCleanupProgress - 1);

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    // 1) Tests migration of tombstones between version 1 and version 2 formats during replica open
    // 2) Tests migration of tombstones between version 1 and version 2 formats on the secondary 
    //    replica after performing a copy
    //
    BOOST_AUTO_TEST_CASE(TombstoneMigrationTest)
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;
        StoreConfig::GetConfig().TombstoneMigrationBatchSize = 7;

        int typeCount = 5;
        int keyCount = 5;
        int bytesCount = 16;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneMigrationTest({0}, {1})", typeCount, keyCount);

        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        // Populate primary with data and initial tombstones
        //
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        primary->UpdateEpoch(1, 0, 0);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);
        replicator->Drain();

        // Replicate initial version 1 tombstones
        //
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->DeleteData(typeCount, keyCount, 1);
        replicator->DrainAndClose();

        primary->VerifyProgress(3);

        auto expectedCount = (typeCount * keyCount);
        
        auto version1TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones1(version1TombstonesData, expectedCount);
        secondary->VerifyData(version1TombstonesData);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to version 1 to version 2 tombstones");

        // Re-open and migrate version 1 to version 2 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        replicator->ResetQueue();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(2, 0, 3);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(2, 0, 3);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3);
        replicator->DrainAndClose();

        auto version2TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(version2TombstonesData, expectedCount);
        secondary->VerifyData(version2TombstonesData);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to version 2 to version 1 tombstones");

        // Re-open and migrate back to version 1 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        replicator->ResetQueue();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(3, 0, 3);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(3, 0, 3);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3);
        replicator->Drain();

        version1TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones1(version1TombstonesData, expectedCount);
        secondary->VerifyData(version1TombstonesData);

        // Insert some version 2 tombstones
        //
        primary->Store.Test_SetEnableTombstoneCleanup2(true);
        secondary->Store.Test_SetEnableTombstoneCleanup2(true);

        wstring type2(L"Type2_");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, type2);
        primary->DeleteData(typeCount, keyCount, 1, type2);
        replicator->DrainAndClose();

        primary->VerifyProgress(7);

        expectedCount += (typeCount * keyCount);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to mixed to version 2 tombstones");

        // Re-open and migrate mixed version tombstones to version 2 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        replicator->ResetQueue();
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(4, 0, 7);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(4, 0, 7);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 7);
        replicator->Drain();

        version2TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(version2TombstonesData, expectedCount);
        secondary->VerifyData(version2TombstonesData);

        // Insert some version 1 tombstones
        //
        primary->Store.Test_SetEnableTombstoneCleanup2(false);
        secondary->Store.Test_SetEnableTombstoneCleanup2(false);

        wstring type3(L"Type3_");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, type3);
        primary->DeleteData(typeCount, keyCount, 1, type3);
        replicator->DrainAndClose();

        primary->VerifyProgress(10);

        expectedCount += (typeCount * keyCount);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to mixed to version 1 tombstones");

        // Re-open and migrate mixed version tombstones to version 1 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        replicator->ResetQueue();
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(5, 0, 10);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(5, 0, 10);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10);
        replicator->DrainAndClose();

        version1TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones1(version1TombstonesData, expectedCount);
        secondary->VerifyData(version1TombstonesData);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to version 2 tombstones after copy");

        // Migrate version 1 tombstones to version 2 tombstones on secondary only after copy
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        replicator->ResetQueue();
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(5, 0, 10);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(5, 0, 10);

        secondary->Store.Test_SetEnableTombstoneCleanup2(true);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10);
        replicator->DrainAndClose();

        version1TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones1(version1TombstonesData, expectedCount);

        version2TombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstones2(version2TombstonesData, expectedCount);

        primary->Close();
        secondary->Close();

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Migrate to version 1 tombstones after copy");

        // Migrate version 2 tombstones to version 1 tombstones on secondary only after copy
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        replicator->ResetQueue();
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(5, 0, 10);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(5, 0, 10);

        secondary->Store.Test_SetEnableTombstoneCleanup2(false);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 10);
        replicator->DrainAndClose();

        version2TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(version2TombstonesData, expectedCount);

        version1TombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstones1(version1TombstonesData, expectedCount);

        primary->Close();
        secondary->Close();
    }

    // Tests that replicas generate the correct tombstone format locally depending on their config
    //
    BOOST_AUTO_TEST_CASE(TombstoneMigrationTest2)
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;
        StoreConfig::GetConfig().TombstoneMigrationBatchSize = 7;

        int typeCount = 5;
        int keyCount = 5;
        int bytesCount = 16;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneMigrationTest2({0}, {1})", typeCount, keyCount);

        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        primary->UpdateEpoch(1, 0, 0);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);

        // Replicate initial version 1 tombstones to secondary configured for version 2 tombstones
        //
        secondary->Store.Test_SetEnableTombstoneCleanup2(true);
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->DeleteData(typeCount, keyCount, 1);
        replicator->DrainAndClose();

        auto expectedCount = (typeCount * keyCount);

        auto version1TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones1(version1TombstonesData, expectedCount);

        auto version2TombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstones2(version2TombstonesData, expectedCount);

        primary->Close();
        secondary->Close();
        
        // Replicate initial version 2 tombstones to secondary configured for version 1 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        replicator->ResetQueue();
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(2, 0, 3);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(2, 0, 3);
        
        secondary->Store.Test_SetEnableTombstoneCleanup2(false);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 3);
        replicator->Drain();

        wstring type2(L"Type2_");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, type2);
        primary->DeleteData(typeCount, keyCount, 1, type2);
        replicator->DrainAndClose();

        expectedCount += (typeCount * keyCount);

        version2TombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(version2TombstonesData, expectedCount);

        version1TombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstones1(version1TombstonesData, expectedCount);

        primary->Close();
        secondary->Close();
    }

    // Tests migration of tombstones generated with the same LSN but non-contiguous
    // during tombstone enumeration
    //
    BOOST_AUTO_TEST_CASE(TombstoneMigrationTest3)
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;
        StoreConfig::GetConfig().TombstoneMigrationBatchSize = 7;

        wstring dataType = L"MigrationTest3Type";
        byte data[] = { 42 };

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneMigrationTest3");

        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        primary->UpdateEpoch(1, 0, 0);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);

        // Replicate initial version 1 tombstones to secondary
        //
        {
            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"A",
                data,
                sizeof(data));

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"B",
                data,
                sizeof(data));

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"C",
                data,
                sizeof(data));

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"D",
                data,
                sizeof(data));

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"E",
                data,
                sizeof(data));

            primary->Store.Insert(
                txSPtr,
                dataType,
                L"F",
                data,
                sizeof(data));

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        // Delete such that tombstones are non-contiguous by LSN
        //
        {
            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"B");

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"D");

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"F");

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {
            TransactionSPtr txSPtr;
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"A");

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"C");

            primary->Store.Delete(
                txSPtr,
                dataType,
                L"E");

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        replicator->DrainAndClose();

        primary->VerifyProgress(4);

        auto expectedCount = 6;

        auto primaryTombstoneData = primary->ReadUserData();
        primary->VerifyTombstones1(primaryTombstoneData, expectedCount);

        auto secondaryTombstoneData = secondary->ReadUserData();
        secondary->VerifyTombstones1(secondaryTombstoneData, expectedCount);

        primary->Close();
        secondary->Close();
        
        // Re-open and migrate version 1 to version 2 tombstones
        //
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        primary = PrimaryReplica::Create(replicator);
        secondary = SecondaryReplica::Create(replicator);
        replicator->ResetQueue();
        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        primary->UpdateEpoch(2, 0, 4);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, ErrorCodeValue::Success, FABRIC_REPLICA_OPEN_MODE_EXISTING);
        secondary->UpdateEpoch(2, 0, 4);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 4);
        replicator->DrainAndClose();

        primaryTombstoneData = primary->ReadUserData();
        primary->VerifyTombstones2(primaryTombstoneData, expectedCount);
        secondary->VerifyData(primaryTombstoneData);

        primary->Close();
        secondary->Close();
    }

    // Tests that version 2 format tombstones can be updated in place even if they already exist.
    // 
    BOOST_AUTO_TEST_CASE(TombstoneUpdate2Test)
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 600;
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;

        int typeCount = 5;
        int keyCount = 5;
        int bytesCount = 16;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneUpdate2Test({0}, {1})", typeCount, keyCount);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        primary->UpdateEpoch(1, 0, 0);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);

        // Generate tombstones at some LSN
        //
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->DeleteData(typeCount, keyCount, 1);
        replicator->Drain();

        primary->VerifyProgress(3);

        // Generate more tombstones re-using LSN
        //
        replicator->ResetOperationLSNs();
        replicator->SetNotificationLsn(3);
        secondary->ResetReplicationNotificationComplete();

        primary->InsertData(1, 1, 1, 1, L"Placeholder_");
        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->DeleteData(typeCount, keyCount, 1);
        replicator->Drain();

        primary->VerifyProgress(3);

        auto expectedCount = (typeCount * keyCount);

        // in-memory tombstone tracking will not know that we
        // are intentionally overwriting LSNs
        //
        auto estimatedCount = expectedCount + (typeCount * keyCount);

        auto tombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(tombstonesData, expectedCount, estimatedCount);
        secondary->VerifyData(tombstonesData);

        // Insert re-delete same keys again with different LSNs
        //
        replicator->SetNotificationLsn(5);
        secondary->ResetReplicationNotificationComplete();

        primary->InsertData(typeCount, keyCount, bytesCount);
        primary->DeleteData(typeCount, keyCount, 1);
        replicator->DrainAndClose();

        expectedCount += (typeCount * keyCount);
        estimatedCount += (typeCount * keyCount);

        tombstonesData = primary->ReadUserData();
        primary->VerifyTombstones2(tombstonesData, expectedCount, estimatedCount);
        secondary->VerifyData(tombstonesData);

        primary->Close();
        secondary->Close();
    }

    // Tests that tombstones are cleaned up in non-decreasing LSN order
    //
    void TombstoneOrderTest();

    BOOST_AUTO_TEST_CASE(TombstoneOrderTest_Version1)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = false;
        TombstoneOrderTest();
    }

    BOOST_AUTO_TEST_CASE(TombstoneOrderTest_Version2)
    {
        StoreConfig::GetConfig().EnableTombstoneCleanup2 = true;
        TombstoneOrderTest();
    }

    void TombstoneOrderTest()
    {
        StoreConfig::GetConfig().TombstoneCleanupLimit = 9;

        int typeCount = 1;
        int keyCount = 1;
        int bytesCount = 16;
        int cleanupLimit = StoreConfig::GetConfig().TombstoneCleanupLimit;
        int cleanupCount = (cleanupLimit + 1) / 2;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TombstoneOrderTest({0}, {1}, v2={2})", typeCount, keyCount, StoreConfig::GetConfig().EnableTombstoneCleanup2);

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        primary->UpdateEpoch(1, 0, 0);
        replicator->QueueCopyOperations(primary->Store, secondary->Store, 0);

        // Generate initial tombstones
        //
        for (auto ix=0; ix<cleanupLimit; ++ix)
        {
            wstring type = wformatString(L"type{0}_", ix);
            primary->InsertData(typeCount, keyCount, bytesCount, 1, type);
            primary->DeleteData(typeCount, keyCount, 1, type);
        }
        replicator->Drain();

        auto startingLsn = 3;
        auto expectedLsn = startingLsn;

        auto tombstonesData = primary->ReadUserData();
        primary->VerifyTombstoneOrder(tombstonesData, expectedLsn, cleanupLimit);

        tombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstoneOrder(tombstonesData, expectedLsn, cleanupLimit);

        // Trigger cleanup
        //
        wstring type(L"type_first_cleanup_");
        primary->InsertData(typeCount, keyCount, bytesCount, 1, type);
        primary->DeleteData(typeCount, keyCount, 1, type);

        auto expectedCount = cleanupCount;
        primary->WaitForDataCount(expectedCount);
        secondary->WaitForDataCount(expectedCount);

        expectedLsn = (cleanupLimit + 1) + startingLsn;
        tombstonesData = primary->ReadUserData();
        primary->VerifyTombstoneOrder(tombstonesData, expectedLsn, expectedCount);

        tombstonesData = secondary->ReadUserData();
        secondary->VerifyTombstoneOrder(tombstonesData, expectedLsn, expectedCount);

        // Trigger more cleanups
        //
        for (auto ix=0; ix<10; ++ix)
        {
            for (auto jx=0; jx<cleanupCount; ++jx)
            {
                wstring keyType = wformatString(L"type{0}_{1}_", ix + cleanupLimit, jx);
                primary->InsertData(typeCount, keyCount, bytesCount, 1, keyType);
                primary->DeleteData(typeCount, keyCount, 1, keyType);

                // Avoid replications too close together for test purposes
                // so that cleanup doesn't get skipped due to an already
                // active cleanup thread from a previous replication.
                //
                Sleep(100);
            }
            replicator->Drain();

            primary->WaitForDataCount(expectedCount);
            secondary->WaitForDataCount(expectedCount);

            expectedLsn += cleanupCount * 2;
            tombstonesData = primary->ReadUserData();
            primary->VerifyTombstoneOrder(tombstonesData, expectedLsn, expectedCount);

            tombstonesData = secondary->ReadUserData();
            secondary->VerifyTombstoneOrder(tombstonesData, expectedLsn, expectedCount);
        }

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    BOOST_AUTO_TEST_CASE(TimeoutCommit)
    {
        byte b[] = { 1, 2, 3 };
        ErrorCode error = ErrorCodeValue::Success;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** TimeoutCommit");

        auto replicator = make_com<ComMockReplicator>(false, 3000); // replication async complete after 3 seconds
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        {
            TransactionSPtr txSPtr1;

            error = primary->Store.CreateTransaction(txSPtr1);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key1", b, sizeof(b));
            error = txSPtr1->Commit(TimeSpan::FromSeconds(5));
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {
            TransactionSPtr txSPtr2;

            error = primary->Store.CreateTransaction(txSPtr2);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr2, L"type", L"key2", b, sizeof(b));
            error = txSPtr2->Commit(TimeSpan::FromSeconds(1));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Timeout));
        }

        {
            TransactionSPtr txSPtr3;

            error = primary->Store.CreateTransaction(txSPtr3);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Update(txSPtr3, L"type", L"key1", 0, L"key1", b, sizeof(b));
            error = txSPtr3->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {	
            TransactionSPtr txSPtr;

            error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr, L"type", L"key", enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr, L"type", L"key11", b, sizeof(b));
            enumSPtr.reset();
            error = txSPtr->Commit();
            txSPtr.reset();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {	
            TransactionSPtr txSPtr;

            error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            IStoreBase::EnumerationSPtr enumSPtr;
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr, L"type", L"key", enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr, L"type", L"key12", b, sizeof(b));
            error = txSPtr->Commit(TimeSpan::FromSeconds(1));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::Timeout));
            enumSPtr.reset();
            Sleep(4000);
            txSPtr.reset();
        }

        replicator->DrainAndClose();

        secondary->SetCopyNotificationComplete();

        auto dataToVerify = primary->ReadUserData();
        secondary->VerifyData(dataToVerify);
        primary->VerifyProgress(5);
        secondary->VerifyProgress(5);

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    BOOST_AUTO_TEST_CASE(EnumerationTest)
    {
        byte b[] = { 1, 2, 3 };
        ErrorCode error = ErrorCodeValue::Success;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** EnumerationTest");

        auto replicator = make_com<ComMockReplicator>(false, 3000); // replication async complete after 3 seconds
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        {
            // Regular transaction enumeration must succeed
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;

            error = primary->Store.CreateTransaction(txSPtr1);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key1", b, sizeof(b));
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr1, L"type", L"key1", enum1);
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        {
            // Simple transaction enumeration must fail
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;

            error = primary->Store.CreateSimpleTransaction(txSPtr1);
            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key2", b, sizeof(b));
            error = primary->Store.CreateEnumerationByTypeAndKey(txSPtr1, L"type", L"key1", enum1);
            VERIFY_IS_TRUE(error.ReadValue() == ErrorCodeValue::InvalidOperation);
        }

        replicator->Close();
        secondary->Close();
        primary->Close();
    }

    BOOST_AUTO_TEST_CASE(InvalidOperations)
    {
        byte b[] = { 1, 2, 3 };
        ErrorCode error = ErrorCodeValue::Success;
        Common::Random r;

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** InvalidOperations");

        auto replicator = make_com<ComMockReplicator>(false, 3000); // replication async complete after 3 seconds
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        {
            // Verify 2 commits fail
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;
            
            error = r.Next() % 2 == 0 ? 
                primary->Store.CreateTransaction(txSPtr1) :
                primary->Store.CreateSimpleTransaction(txSPtr1);

            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key1", b, sizeof(b));

            error = txSPtr1->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());

            error = txSPtr1->Commit();
            VERIFY_IS_TRUE(error.ToHResult() == FABRIC_E_TRANSACTION_NOT_ACTIVE);
        }

        {
            // Verify commit after a rollback fails
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;

            error = r.Next() % 2 == 0 ? 
                primary->Store.CreateTransaction(txSPtr1) :
                primary->Store.CreateSimpleTransaction(txSPtr1);

            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key2", b, sizeof(b));

            txSPtr1->Rollback();

            error = txSPtr1->Commit();
            VERIFY_IS_TRUE(error.ToHResult() == FABRIC_E_TRANSACTION_NOT_ACTIVE);
        }

        {
            // Verify 2 rollbacks does not crash
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;

            error = r.Next() % 2 == 0 ? 
                primary->Store.CreateTransaction(txSPtr1) :
                primary->Store.CreateSimpleTransaction(txSPtr1);

            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key3", b, sizeof(b));

            txSPtr1->Rollback();
            txSPtr1->Rollback();
        }

        {
            // Verify rollback after commit does not crash
            TransactionSPtr txSPtr1;
            EnumerationSPtr enum1;

            error = r.Next() % 2 == 0 ? 
                primary->Store.CreateTransaction(txSPtr1) :
                primary->Store.CreateSimpleTransaction(txSPtr1);

            VERIFY_IS_TRUE(error.IsSuccess());

            primary->Store.Insert(txSPtr1, L"type", L"key4", b, sizeof(b));

            error = txSPtr1->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());

            txSPtr1->Rollback();
        }

        replicator->Close();
        primary->Close();
        secondary->Close();
    }

    BOOST_AUTO_TEST_CASE(PerformanceBaselineTest)
    {
        bool isEnabled = false;

        int keyCount = 10 * 1024;
        int byteCount = 8;

        if (isEnabled)
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PerformanceBaselineTest");
        }
        else
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** PerformanceBaselineTest (disabled)");

            return;
        }

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        vector<byte> data;
        for (int ix=0; ix<byteCount; ++ix)
        {
            data.push_back(static_cast<byte>(ix));
        }

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);

        primary->InsertData(1, 1, byteCount, 1, L"InitType", L"InitKey");

        Stopwatch stopwatch;

        stopwatch.Start();

        for (int ix=0; ix<keyCount; ++ix)
        {
            primary->InsertData(1, 1, byteCount, 1, L"PerfType", wformatString("PerfKey_{0}", ix));
        }

        auto insertionTime = stopwatch.Elapsed;

        for (int ix=0; ix<keyCount; ++ix)
        {
            primary->UpdateData(1, 1, byteCount, 1, L"PerfType", wformatString("PerfKey_{0}", ix));
        }

        stopwatch.Stop();

        auto updateTime = stopwatch.Elapsed;

        replicator->ResetQueue();
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        secondary->VerifyProgress(0);

        secondary->UpdateEpoch(1, 0, 0);

        replicator->QueueCopyOperations(primary->Store, secondary->Store, 1);
        replicator->DrainAndClose();
        secondary->VerifyProgress(1);
        
        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        stopwatch.Start();

        replicator->QueueCopyOperations(primary->Store, secondary->Store, keyCount * 2 + 1);
        replicator->DrainAndClose();

        stopwatch.Stop();

        auto partialCopyTime = stopwatch.Elapsed;
        
        secondary->VerifyProgress(keyCount * 2 + 1);

        secondary->Close();

        replicator->ResetQueue();
        secondary = SecondaryReplica::Create(replicator);
        secondary->Open(
            ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY, 
            ErrorCodeValue::Success, 
            FABRIC_REPLICA_OPEN_MODE_EXISTING);

        secondary->UpdateEpoch(2, 0, 0);

        stopwatch.Start();

        replicator->QueueCopyOperations(primary->Store, secondary->Store, keyCount * 2 + 1, ErrorCodeValue::Success, false);
        replicator->QueueLastCopyOperation();
        replicator->DrainAndClose();

        stopwatch.Stop();

        auto fullCopyTime = stopwatch.Elapsed;
        
        secondary->VerifyProgress(keyCount * 2 + 1);

        secondary->Close();
        primary->Close();

        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "Results: insert={0}/{1} update={2}/{3} partialCopy={4}/{5} fullCopy={6}/{7}",
            insertionTime,
            (double)keyCount / insertionTime.TotalMilliseconds() * 1000,
            updateTime,
            (double)keyCount / updateTime.TotalMilliseconds() * 1000,
            partialCopyTime,
            (double)keyCount / partialCopyTime.TotalMilliseconds() * 1000,
            fullCopyTime,
            (double)keyCount / fullCopyTime.TotalMilliseconds() * 1000);
    }

    // Multi-threaded transactions are not supported. Test that incorrect usage results
    // in errors returned to the caller, but not asserts or AVs.
    //
    BOOST_AUTO_TEST_CASE(MultithreadedTransactionsTest)
    {
        byte b[] = { 1, 2, 3 };
        int runs = 20;
        int iterations = 100;

        auto type = L"type";
        auto key1 = L"key1";
        auto key2 = L"key2";

        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "*** MultithreadedTransactionsTest");

        auto replicator = make_com<ComMockReplicator>();
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();

        // Setup for updates
        {
            TransactionSPtr txSPtr;
            
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, type, key1, b, sizeof(b));
            VERIFY_IS_TRUE(error.IsSuccess());

            error = primary->Store.Insert(txSPtr, type, key2, b, sizeof(b));
            VERIFY_IS_TRUE(error.IsSuccess());

            error = txSPtr->Commit();
            VERIFY_IS_TRUE(error.IsSuccess());
        }

        // Parallel operations can fail but shouldn't assert
        //
        // JET_errUpdateNotPrepared -> StoreUnexpectedError
        //
        for (auto rx=0; rx<runs; ++rx)
        {
            TransactionSPtr txSPtr;
            
            auto error = primary->Store.CreateTransaction(txSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            ManualResetEvent thread1(false);
            ManualResetEvent thread2(false);

            Threadpool::Post([&]()
                {
                    for (auto ix=0; ix<iterations; ++ix)
                    {
                        auto innerErr = primary->Store.Update(txSPtr, type, key1, 0 , key1, b, sizeof(b));

                        if (innerErr.IsError(ErrorCodeValue::MultithreadedTransactionsNotAllowed) || 
                            innerErr.IsError(ErrorCodeValue::TransactionAborted) ||
                            innerErr.IsError(ErrorCodeValue::StoreRecordNotFound) ||
                            innerErr.IsError(ErrorCodeValue::StoreUnexpectedError) ||
                            innerErr.IsError(ErrorCodeValue::StoreWriteConflict))
                        {
                            break;
                        }
                        else
                        {
                            VERIFY(innerErr.IsSuccess(), "error={0}", innerErr);
                        }
                    }

                    thread1.Set();
                });

            Threadpool::Post([&]()
                {
                    for (auto ix=0; ix<iterations; ++ix)
                    {
                        auto innerErr = primary->Store.Update(txSPtr, type, key2, 0 , key2, b, sizeof(b));

                        if (innerErr.IsError(ErrorCodeValue::MultithreadedTransactionsNotAllowed) ||
                            innerErr.IsError(ErrorCodeValue::TransactionAborted) ||
                            innerErr.IsError(ErrorCodeValue::StoreRecordNotFound) ||
                            innerErr.IsError(ErrorCodeValue::StoreUnexpectedError) ||
                            innerErr.IsError(ErrorCodeValue::StoreWriteConflict))
                        {
                            break;
                        }
                        else
                        {
                            VERIFY(innerErr.IsSuccess(), "error={0}", innerErr);
                        }
                    }

                    thread2.Set();
                });

            thread1.WaitOne();
            thread2.WaitOne();

            txSPtr->Rollback();
        }

        replicator->DrainAndClose();
        secondary->Close();
        primary->Close(ErrorCodeValue::StoreInUse);
    }

    BOOST_AUTO_TEST_CASE(SimpleFabricTimerTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"*** SimpleFabricTimerTest starts");

        FabricTimeData fabricWatch;
        int64 start=fabricWatch.RefreshAndGetCurrentStoreTime();
        int64 then=fabricWatch.RefreshAndGetCurrentStoreTime();
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Start:{0},then:{1}",start,then);
        fabricWatch.Start();
        Sleep(100);
        then=fabricWatch.RefreshAndGetCurrentStoreTime();
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Start:{0},then:{1}",start,then);
        VERIFY_IS_TRUE(start<then);
        Sleep(20);
        int64 end=fabricWatch.RefreshAndGetCurrentStoreTime();
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Start:{0},then:{1},end:{2}",start,then,end);
        VERIFY_IS_TRUE(then<end);   
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"SimpleFabricTimerTest ends");
    }

    BOOST_AUTO_TEST_CASE(FabricTimerReplicationTest)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"*** FabricTimerReplicationTest starts");
        ErrorCode error(ErrorCodeValue::Success);
        auto replicator = make_com<ComMockReplicator>(false, 100); // replication async complete after 0.1 seconds
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);

        primary->Open(::FABRIC_REPLICA_ROLE_PRIMARY);
        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->QueueLastCopyOperation();
        StoreConfig::GetConfig().set_FabricTimePersistInterval(TimeSpan::FromSeconds(1));
        int64 t1,t2,t3,t4,t5,t6;
        {
            FabricTimeControllerSPtr fbcSPtr=std::make_shared<FabricTimeController>(primary->Store);
            fbcSPtr->Start(true);
            do{
                error=fbcSPtr->GetCurrentStoreTime(t1);
                Sleep(100);
            }while(!error.IsSuccess());

            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 1, t1:{0}, Error:{1}",t1,error);            
            Sleep(1000);
            error=fbcSPtr->GetCurrentStoreTime(t2);
            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 1, t1:{0}, Error:{1}",t2,error);            
            Sleep(2000);
            error=fbcSPtr->GetCurrentStoreTime(t3);
            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 1, t1:{0}, Error:{1}",t3,error);  
            fbcSPtr->Cancel();
        }        
        {
            FabricTimeControllerSPtr fbcSPtr=std::make_shared<FabricTimeController>(primary->Store);
            fbcSPtr->Start(true);
            do{
                error=fbcSPtr->GetCurrentStoreTime(t4);
                Sleep(100);
            }while(!error.IsSuccess());
            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 2, t1:{0}, Error:{1}",t4,error);               
            Sleep(1000);
            error=fbcSPtr->GetCurrentStoreTime(t5);
            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 2, t1:{0}, Error:{1}",t5,error);            
            Sleep(2000);
            error=fbcSPtr->GetCurrentStoreTime(t6);
            Trace.WriteInfo(ReplicatedStoreTest::TestSource,"Round 2, t1:{0}, Error:{1}",t6,error);     
            fbcSPtr->Cancel();
        }
        VERIFY_IS_TRUE(t1<t2 && t2<t3);
        VERIFY_IS_TRUE(t4<t5 && t5<t6);
        int64 gap;
        gap=TimeSpan::FromTicks(t2-t1).TotalMilliseconds();
        VERIFY_IS_TRUE(gap<1500 && gap>500);
        gap=TimeSpan::FromTicks(t3-t2).TotalMilliseconds();
        VERIFY_IS_TRUE(gap<2500 && gap>1500);
        gap=TimeSpan::FromTicks(t4-t3).TotalMilliseconds();
        gap=TimeSpan::FromTicks(t5-t4).TotalMilliseconds();
        VERIFY_IS_TRUE(gap <StoreConfig::GetConfig().FabricTimePersistInterval.get_Ticks());//the gap is caused by refresh interval and replication delay. 
        gap=TimeSpan::FromTicks(t5-t4).TotalMilliseconds();
        VERIFY_IS_TRUE(gap<1500 && gap>500);
        gap=TimeSpan::FromTicks(t6-t5).TotalMilliseconds();
        VERIFY_IS_TRUE(gap<2500 && gap>1500);

        // Timer may fire after close
        //
        replicator->SetVerifyReplicationSuccess(false);

        replicator->Close();
        primary->Close();
        secondary->Close();
        StoreConfig::GetConfig().FabricTimePersistInterval=TimeSpan::Zero;
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"FabricTimerReplicationTest ends");
    }

    BOOST_AUTO_TEST_CASE(FabricTimerFailoverTest)
    {
        StoreConfig::GetConfig().FabricTimePersistInterval=TimeSpan::FromSeconds(1);
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"*** FabricTimerFailoverTest starts");
        auto replicator = make_com<ComMockReplicator>(false, 100); // replication async complete after 0.1 seconds
        PrimaryReplicaSPtr primary = PrimaryReplica::Create(replicator);
        SecondaryReplicaSPtr secondary = SecondaryReplica::Create(replicator);
        
        primary->Open(::FABRIC_REPLICA_ROLE_NONE,ErrorCodeValue::Success,FABRIC_REPLICA_OPEN_MODE_NEW);
        primary->Store.Test_SetStartFabricTimerWithCancelWait(true);
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

        primary->UpdateEpoch(1, 0, 0);
        secondary->Open(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY,ErrorCodeValue::Success,FABRIC_REPLICA_OPEN_MODE_NEW);
        replicator->QueueLastCopyOperation();

        int64 t1,t2,t3,t4,t5,t6;
        int64 gap;
        ErrorCode error(ErrorCodeValue::Success);
        do{
            error=primary->Store.GetCurrentStoreTime(t1);
            Sleep(100);
        }while(!error.IsSuccess());
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t1:{0} error?{1}",t1,error);   
        VERIFY_IS_TRUE(error.IsSuccess());
        Sleep(1000);
        error=primary->Store.GetCurrentStoreTime(t2);
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t2:{0} error?{1}",t2,error);   
        gap=TimeSpan::FromTicks(t2-t1).TotalMilliseconds();
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(gap<1500 && gap>500);
        error=secondary->Store.GetCurrentStoreTime(t3);
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t3:{0} error?{1}",t3,error);   
        VERIFY_IS_TRUE(!error.IsSuccess());
        Sleep(2000);
        error=primary->Store.GetCurrentStoreTime(t4);
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t4:{0} error?{1}",t4,error);   
        gap=TimeSpan::FromTicks(t4-t2).TotalMilliseconds();
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(gap<2500 && gap>1500);

        //Swap primary and secondary
        
        std::swap(primary, secondary);
        secondary->ChangeRole(::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
        replicator->DrainAndClose();
        replicator->ResetQueue();
        primary->Store.Test_SetStartFabricTimerWithCancelWait(true);
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_PRIMARY);

        primary->UpdateEpoch(2, 0, 0);
        secondary->UpdateEpoch(2, 0, 0);
        
        do{
            error=primary->Store.GetCurrentStoreTime(t5);
            Sleep(100);
        }while(!error.IsSuccess());
        
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t5:{0} error?{1}",t5,error);   
        gap=TimeSpan::FromTicks(t4-t5).TotalMilliseconds();
        VERIFY_IS_TRUE(error.IsSuccess());
        VERIFY_IS_TRUE(gap<StoreConfig::GetConfig().FabricTimePersistInterval.get_Ticks());
        error=secondary->Store.GetCurrentStoreTime(t6);
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"t6:{0} error?{1}",t6,error);   
        VERIFY_IS_TRUE(!error.IsSuccess());
        primary->ChangeRole(::FABRIC_REPLICA_ROLE_NONE);

        // Timer may fire after close
        //
        replicator->SetVerifyReplicationSuccess(false);
        replicator->Close();
        primary->Close();
        secondary->Close();
        StoreConfig::GetConfig().FabricTimePersistInterval=TimeSpan::Zero;
        Trace.WriteInfo(ReplicatedStoreTest::TestSource,"FabricTimerFailoverTest ends");
    }

    BOOST_AUTO_TEST_SUITE_END()
	
    // ************************* 
    // Helper (member) functions
    // *************************
    
    // ****************************** 
    // BackupRestoreHelper functions
    // ******************************

    BackupRestoreHelper::BackupRestoreHelper() 
        : backupBaseDir_(Path::Combine(Directory::GetCurrentDirectoryW(), L"BackupBaseDir"))
        , backupDirFull_(Path::Combine(backupBaseDir_, L"BackupFull"))
        , backupDirInc_(Path::Combine(backupBaseDir_, L"BackupIncremental"))
        , restoreDir_(Path::Combine(Directory::GetCurrentDirectoryW(), L"RestoreDir"))
        , restoreDirFull_(Path::Combine(restoreDir_, L"BackupFull"))
        , restoreDirInc_(Path::Combine(restoreDir_, L"BackupIncremental"))
        , backupHandler_(new PostBackupHandler())
        , incBackupCount_(10)
    {       
    }

    BackupRestoreHelper::~BackupRestoreHelper()
    {
        Directory::Delete(backupBaseDir_, true);
        Directory::Delete(restoreDir_, true);
    }

    void BackupRestoreHelper::InserDataAndTakeBackups(
        ReplicaBaseSPtr replica,
        std::vector<ReplicaBase::DataItem> & origData1,
        std::vector<ReplicaBase::DataItem> & origData2)
    {
        this->InsertRandomData(replica);

        Trace.WriteInfo(TraceType, "Taking full backups...");

        CODING_ERROR_ASSERT(this->TakeBackup(replica, this->backupDirFull_, StoreBackupOption::Full).IsSuccess());

        Trace.WriteInfo(TraceType, "Taking multiple incremental backups...");

        for (_int64 i = 1; i <= incBackupCount_; i++)
        {
            this->InsertRandomData(replica);
            CODING_ERROR_ASSERT(this->TakeBackup(replica, wformatString("{0}{1}", this->backupDirInc_, i), StoreBackupOption::Incremental).IsSuccess());

            if (i == incBackupCount_ - 1)
            {
                origData1 = replica->ReadUserData();
            }
        }

        origData2 = replica->ReadUserData();

        Trace.WriteInfo(TraceType, "Completed multiple incremental backups.");
    }

    void BackupRestoreHelper::VerifyEmptyDirectoryValidation(ReplicaBaseSPtr replica)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: Empty directory.");

        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_).IsSuccess());

        std::wstring mergedRestoreDir = L"";
        auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::FileNotFound));
        
        CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
    }

    void BackupRestoreHelper::VerifyInvalidBackupChainValidation1(ReplicaBaseSPtr replica)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: InvalidBackupChain - 1.");

#if defined(PLATFORM_UNIX)
        Directory::Create2(this->restoreDir_);
#endif
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"1", this->restoreDir_ + L"\\Inc1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc2").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"2", this->restoreDir_ + L"\\Inc2", true).IsSuccess());

        std::wstring mergedRestoreDir = L"";
        auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::InvalidBackupChain));

        CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
    }

    void BackupRestoreHelper::VerifyDuplicateFullBackupValidation1(ReplicaBaseSPtr replica)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: DuplicateBackups - 1.");

#if defined(PLATFORM_UNIX)
        Directory::Create2(this->restoreDir_);
#endif
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full2").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full2", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"1", this->restoreDir_ + L"\\Inc1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc2").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"2", this->restoreDir_ + L"\\Inc2", true).IsSuccess());

        std::wstring mergedRestoreDir = L"";
        auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::DuplicateBackups));

        CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
    }

    void BackupRestoreHelper::VerifyDuplicateFullBackupValidation2(ReplicaBaseSPtr replica)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: DuplicateBackups - 2.");

#if defined(PLATFORM_UNIX)
        Directory::Create2(this->restoreDir_);
#endif
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"1", this->restoreDir_ + L"\\Inc1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc2").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"2", this->restoreDir_ + L"\\Inc2", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc3").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"2", this->restoreDir_ + L"\\Inc3", true).IsSuccess());

        std::wstring mergedRestoreDir = L"";
        auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::DuplicateBackups));

        CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
    }

    void BackupRestoreHelper::VerifyInvalidBackupChainValidation2(ReplicaBaseSPtr replica)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: InvalidBackupChain - 2.");

#if defined(PLATFORM_UNIX)
        Directory::Create2(this->restoreDir_);
#endif
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc1").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"1", this->restoreDir_ + L"\\Inc1", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc2").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"2", this->restoreDir_ + L"\\Inc2", true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc4").IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"4", this->restoreDir_ + L"\\Inc4", true).IsSuccess());

        std::wstring mergedRestoreDir = L"";
        auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::InvalidBackupChain));

        CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
    }

	void BackupRestoreHelper::VerifyBackupWithMissingFile(ReplicaBaseSPtr replica)
	{
		Trace.WriteInfo(TraceType, "Backup chain validation. Case: InvalidBackup with MissingFile.");

		CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full1").IsSuccess());
		CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full1", true).IsSuccess());

		CODING_ERROR_ASSERT(File::Delete2(this->restoreDir_ + L"\\Full1\\new\\ReplicatedStoreData.edb").IsSuccess());
		CODING_ERROR_ASSERT(File::Delete2(this->restoreDir_ + L"\\Full1\\new\\ReplicatedStoreData.pat").IsSuccess());

		std::wstring mergedRestoreDir = L"";
		auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
		CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::InvalidBackup));

		CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
	}

	void BackupRestoreHelper::VerifyBackupWithExtraFile(ReplicaBaseSPtr replica)
	{
		Trace.WriteInfo(TraceType, "Backup chain validation. Case: InvalidBackup with ExtraFile.");

		CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Full1").IsSuccess());
		CODING_ERROR_ASSERT(Directory::Copy(this->backupDirFull_, this->restoreDir_ + L"\\Full1", true).IsSuccess());
		CODING_ERROR_ASSERT(Directory::Create2(this->restoreDir_ + L"\\Inc1").IsSuccess());
		CODING_ERROR_ASSERT(Directory::Copy(this->backupDirInc_ + L"1", this->restoreDir_ + L"\\Inc1", true).IsSuccess());

		CODING_ERROR_ASSERT(File::Copy(this->restoreDir_ + L"\\Full1\\new\\ReplicatedStoreData.edb", this->restoreDir_ + L"\\Inc1\\ReplicatedStoreData.edb").IsSuccess());
		CODING_ERROR_ASSERT(File::Copy(this->restoreDir_ + L"\\Full1\\new\\ReplicatedStoreData.pat", this->restoreDir_ + L"\\Inc1\\ReplicatedStoreData.pat").IsSuccess());

		std::wstring mergedRestoreDir = L"";
		auto error = replica->Store.Test_ValidateAndMergeBackupChain(this->restoreDir_, mergedRestoreDir);
		CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::InvalidBackup));

		CODING_ERROR_ASSERT(Directory::Delete(this->restoreDir_, true).IsSuccess());
	}

    void BackupRestoreHelper::VerifyRestoreFromBackupChain(
        ReplicaBaseSPtr newReplica,
        std::vector<ReplicaBase::DataItem> const & origData1,
        std::vector<ReplicaBase::DataItem> const & origData2)
    {
        Trace.WriteInfo(TraceType, "Backup chain validation. Case: Restore from BackupChain.");

        newReplica->Store.Test_SetDatalossCallbackActive(true);

        CODING_ERROR_ASSERT(Directory::Copy(backupBaseDir_, restoreDir_, true).IsSuccess());

        CODING_ERROR_ASSERT(this->DoRestore(newReplica, RestoreSettings(true, true)).IsSuccess());

        auto restoredData = newReplica->ReadUserData();
        newReplica->VerifyEqualData(origData2, restoredData, 0, numeric_limits<_int64>::max());

        CODING_ERROR_ASSERT(Directory::Delete(restoreDir_, true).IsSuccess());

        // Create backup with older than current data in service.
        CODING_ERROR_ASSERT(Directory::Copy(backupBaseDir_, this->restoreDir_, true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Delete(wformatString("{0}{1}", restoreDirInc_, incBackupCount_), true).IsSuccess());

        // Verify safe restore fails with older data
        auto error = this->DoRestore(newReplica, RestoreSettings(true, true));
        CODING_ERROR_ASSERT(error.IsError(ErrorCodeValue::RestoreSafeCheckFailed));

        // Full backup directory is used for merging all backups. Reset it.
        CODING_ERROR_ASSERT(Directory::Delete(restoreDirFull_, true).IsSuccess());
        CODING_ERROR_ASSERT(Directory::Copy(backupDirFull_, restoreDirFull_, true).IsSuccess());

        // Verify restore succeeds with older data without safe option.
        CODING_ERROR_ASSERT(this->DoRestore(newReplica, RestoreSettings(true, false)).IsSuccess());

        restoredData = newReplica->ReadUserData();
        newReplica->VerifyEqualData(origData1, restoredData, 0, numeric_limits<_int64>::max());

        CODING_ERROR_ASSERT(Directory::Delete(restoreDir_, true).IsSuccess());

        // Verify safe restore succeeds with newer data
        CODING_ERROR_ASSERT(Directory::Copy(backupBaseDir_, restoreDir_, true).IsSuccess());

        CODING_ERROR_ASSERT(this->DoRestore(newReplica, RestoreSettings(true, true)).IsSuccess());

        restoredData = newReplica->ReadUserData();
        newReplica->VerifyEqualData(origData2, restoredData, 0, numeric_limits<_int64>::max());

        CODING_ERROR_ASSERT(Directory::Delete(restoreDir_, true).IsSuccess());

        newReplica->Store.Test_SetDatalossCallbackActive(false);
    }

    ErrorCode BackupRestoreHelper::TakeBackup(ReplicaBaseSPtr replica, wstring const & backupDir, StoreBackupOption::Enum backupOption)
    {
        Api::IStorePostBackupHandlerPtr storeBackupHandler(backupHandler_.get(), backupHandler_->CreateComponentRoot());

        ManualResetEvent waiterInc;
        auto operation = replica->Store.BeginBackupLocal(
            backupDir,
            backupOption,
            storeBackupHandler,
            StoreBackupRequester::User,
            [&](AsyncOperationSPtr const &) { waiterInc.Set(); },
            nullptr);

        waiterInc.WaitOne();
        
        return replica->Store.EndBackupLocal(operation);
    }

    ErrorCode BackupRestoreHelper::TakeBackup(ReplicaBaseSPtr replica, StoreBackupOption::Enum backupOption)
    {
        auto backupDir = Path::Combine(backupBaseDir_, Guid::NewGuid().ToString());        
        return this->TakeBackup(replica, backupDir, backupOption);
    }

    ErrorCode BackupRestoreHelper::DoRestore(ReplicaBaseSPtr replica, RestoreSettings restoreSettings)
    {
        ManualResetEvent restoreWaiter;
        auto operation = replica->Store.BeginRestoreLocal(
            this->restoreDir_,
            restoreSettings,
            [&](AsyncOperationSPtr const &) { restoreWaiter.Set(); },
            nullptr);

        restoreWaiter.WaitOne();
        
        return replica->Store.EndRestoreLocal(operation);
    }

    void BackupRestoreHelper::InsertRandomData(ReplicaBaseSPtr replica)
    {
        int typeCount = 10;
        int keyCount = 10;
        int bytesCount = 128;
        auto keyAndTypePrefix = Guid::NewGuid().ToString();

        replica->InsertData(typeCount, keyCount, bytesCount, 1, keyAndTypePrefix, keyAndTypePrefix);
    }

    // ***************** 
    // ComMockReplicator
    // *****************

    void ComMockReplicator::SetVerifyReplicationSuccess(bool value)
    {
        verifyReplicationSuccess_ = value;
    }

    void ComMockReplicator::ResetQueue()
    { 
        if (replicationQueueSPtr_) 
        { 
            replicationQueueSPtr_->Close(); 
        }

        if (copyQueueSPtr_) 
        { 
            copyQueueSPtr_->Close(); 
        }

        replicationQueueSPtr_ = ReplicationQueue::Create(); 
        copyQueueSPtr_ = ReplicationQueue::Create(); 

        notificationLSN_.store(0);

        // intentionally do not reset the replication sequence number count
    }

    void ComMockReplicator::ResetOperationLSNs(LONG lsn)
    { 
        operationLSN_.store(lsn);
        this->SetNotificationLsn(lsn);
    }

    void ComMockReplicator::SetNotificationLsn(LONG lsn)
    { 
        notificationLSN_.store(lsn);
    }

    void ComMockReplicator::Drain()
    {
        this->DrainCopy();

        while (replicationQueueSPtr_->ItemCount > 0) 
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Waiting for replication queue to drain: item count = {0}", replicationQueueSPtr_->ItemCount);
            Sleep(500);
        }        
    }

    void ComMockReplicator::DrainCopy()
    {
        while (copyQueueSPtr_->ItemCount > 0) 
        {
            Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Waiting for copy queue to drain: item count = {0}", copyQueueSPtr_->ItemCount);
            Sleep(500);
        }
    }

    void ComMockReplicator::DrainAndClose()
    {
        this->Drain();

        // Pump NULL to secondary
        this->Close();

        // There is no ACK for the NULL. Give time for secondary apply to commit.
        Sleep(500);
    }

    void ComMockReplicator::QueueLastCopyOperation()
    {
        // Enqueue the null copy operation
        bool success = copyQueueSPtr_->EnqueueAndDispatch(nullptr);
        VERIFY_IS_TRUE(success);
    }

    void ComMockReplicator::QueueCopyOperations(
        __in ReplicatedStore & primaryStore,
        __in ReplicatedStore & secondaryStore,
        ::FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
        ErrorCodeValue::Enum expectedError,
        bool queueLastCopyOperation,
        int maxCount)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "Copying up to sequence number {0}", uptoSequenceNumber);

        ComPointer<IFabricOperationDataStream> copyContext;
        ErrorCode error = secondaryStore.GetCopyContext(/*out*/ copyContext);
        VERIFY(error.IsSuccess(), "QueueCopyOperations(): GetCopycontext() failed with {0}", error);

        ComPointer<IFabricOperationDataStream> enumerator;
        error = primaryStore.GetCopyState(uptoSequenceNumber, copyContext, /*out*/ enumerator);
        VERIFY(error.IsSuccess(), "QueueCopyOperations(): GetCopyState() failed with {0}", error);

        AutoResetEvent waiter(false);
        bool done = false;
        int count = 0;

        while (!done)
        {
            auto operation = AsyncOperation::CreateAndStart<ComGetNextAsyncOperation>(
                enumerator,
                [this, &waiter](AsyncOperationSPtr const &) { waiter.Set(); },
                AsyncOperationSPtr());
            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)));

            ComPointer<IFabricOperationData> copyOperation;
            error = ComGetNextAsyncOperation::End(operation, copyOperation);
            VERIFY_IS_TRUE(error.IsError(expectedError), L"QueueCopyOperations(): GetNext()");

            if (copyOperation.GetRawPointer() == NULL || 
                expectedError != ErrorCodeValue::Success ||
                ++count >= maxCount)
            {
                done = true;
            }
            else
            {
                unique_ptr<QueuedReplicationOperation> replicationOperation = make_unique<QueuedReplicationOperation>(
                    ::FABRIC_OPERATION_TYPE_NORMAL,
                    ++ operationLSN_,
                    std::move(copyOperation));

                bool success = copyQueueSPtr_->EnqueueAndDispatch(std::move(replicationOperation));
                VERIFY_IS_TRUE(success);
            }
        }

        notificationLSN_.store(static_cast<LONG>(uptoSequenceNumber));

        if (queueLastCopyOperation)
        {
            QueueLastCopyOperation();
        }
    }

    HRESULT STDMETHODCALLTYPE ComMockReplicator::BeginReplicate(
        __in IFabricOperationData * operation,
        __in IFabricAsyncOperationCallback * callback,
        __out ::FABRIC_SEQUENCE_NUMBER * sequenceNumber,
        __out IFabricAsyncOperationContext ** context)
    {
        if (callback == NULL || context == NULL) { return E_POINTER; }

        *sequenceNumber = ++operationLSN_;

        if (notificationLSN_.load() < operationLSN_.load())
        {
            notificationLSN_.store(operationLSN_.load());
        }

        ComPointer<IFabricOperationData> operationData;
        HRESULT hr = operation->QueryInterface(IID_IFabricOperationData, reinterpret_cast<void**>(operationData.InitializationAddress()));
        ASSERT_IF(!SUCCEEDED(hr), "ComReplicatedOperationData does not support IFabricOperationData");

        unique_ptr<QueuedReplicationOperation> replicationOperation = make_unique<QueuedReplicationOperation>(
            ::FABRIC_OPERATION_TYPE_NORMAL,
            *sequenceNumber,
            std::move(operationData));

        // Create IFabricAsyncOperationContext wrapper
        ComPointer<IFabricAsyncOperationCallback> callbackCPtr(callback, IID_IFabricAsyncOperationCallback);
        ComPointer<ComReplicatedOperationContext> replicationContext = make_com<ComReplicatedOperationContext>(
            replicationQueueSPtr_,
            callbackCPtr);
        hr = replicationContext->QueryInterface(IID_IFabricAsyncOperationContext, reinterpret_cast<void**>(context));
        ASSERT_IF(!SUCCEEDED(hr), "ComReplicatedOperationContext does not support IFabricAsyncOperationContext");

        // Actual operation is a synchronous enqueue
        replicationContext->EnqueueAndDispatch(
            std::move(replicationOperation), 
            replicateSynchronously_, 
            delayReplicateInMilliseconds_,
            verifyReplicationSuccess_);

        return hr;
    }

    HRESULT STDMETHODCALLTYPE ComMockReplicator::EndReplicate(
        __in IFabricAsyncOperationContext * context,
        __out ::FABRIC_SEQUENCE_NUMBER *
        )
    {
        if (context == NULL) { return E_POINTER; }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ComMockReplicator::GetCopyStream(
        __out IFabricOperationStream **stream)
    {
        if (stream == NULL) { return E_POINTER; }
        auto streamCPtr = make_com<ComMockOperationStream, IFabricOperationStream>(copyQueueSPtr_);
        *stream = streamCPtr.DetachNoRelease();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ComMockReplicator::GetReplicationStream(
        __out IFabricOperationStream **stream)
    {
        if (stream == NULL) { return E_POINTER; }
        auto streamCPtr = make_com<ComMockOperationStream, IFabricOperationStream>(replicationQueueSPtr_);
        *stream = streamCPtr.DetachNoRelease();
        return S_OK;
    }

    // ***************** 
    // ComMockReplicator::ComMockOperationStream
    // *****************

    HRESULT STDMETHODCALLTYPE ComMockReplicator::ComMockOperationStream::BeginGetOperation(
        __in  IFabricAsyncOperationCallback * callback,
        __out IFabricAsyncOperationContext ** context)
    {
        ComPointer<IFabricAsyncOperationCallback> callbackCPtr(callback, IID_IFabricAsyncOperationCallback);
        ComPointer<ComReplicatedOperationContext> operationContext = make_com<ComMockReplicator::ComReplicatedOperationContext>(queueSPtr_, callbackCPtr);
        HRESULT hr = operationContext->QueryInterface(IID_IFabricAsyncOperationContext, reinterpret_cast<void**>(context));
        ASSERT_IF(!SUCCEEDED(hr), "ComReplicatedOperationContext does not support IFabricAsyncOperationContext");

        operationContext->BeginDequeue();

        return hr;
    }

    HRESULT STDMETHODCALLTYPE ComMockReplicator::ComMockOperationStream::EndGetOperation(
        __in IFabricAsyncOperationContext * context,
        __out IFabricOperation ** operation)
    {
        if (context == NULL || operation == NULL) { return E_POINTER; }

        ComPointer<ComReplicatedOperationContext> casted;
        HRESULT hr = context->QueryInterface(IID_IFabricAsyncOperationContext, reinterpret_cast<void**>(casted.InitializationAddress()));
        VERIFY_IN_THREAD(SUCCEEDED(hr), "ComReplicatedOperationContext does not support IFabricAsyncOperationContext: hr = {0}", hr);

        ComPointer<IFabricOperation> result;
        casted->EndDequeue(result);

        if (result.GetRawPointer() != NULL)
        {
            result->QueryInterface(IID_IFabricOperation, reinterpret_cast<void**>(operation));
        }

        return hr;
    }

    // ***************** 
    // ComMockReplicator::ComReplicatedOperationContext
    // *****************

    void ComMockReplicator::ComReplicatedOperationContext::EnqueueAndDispatch(
        unique_ptr<QueuedReplicationOperation> && replicationOperation,
        bool synchronous,
        DWORD delayReplicateInMilliseconds,
        bool verifySuccess)
    {
        bool success = queueSPtr_->EnqueueAndDispatch(std::move(replicationOperation));

        if (verifySuccess)
        {
            VERIFY_IS_TRUE(success, L"EnqueueAndDispatch()");
        }

        this->Complete(synchronous, delayReplicateInMilliseconds);
    }

    void ComMockReplicator::ComReplicatedOperationContext::BeginDequeue()
    {
        ComPointer<IFabricAsyncOperationContext> thisCPtr(this, IID_IFabricAsyncOperationContext);
        auto operation = queueSPtr_->BeginDequeue(
            TimeSpan::MaxValue,
            [this, thisCPtr](AsyncOperationSPtr const & operation) 
        { 
            this->OnDequeueComplete(operation);
        },
            // Not really needed since thisCPtr is captured in lambda state
            AsyncOperationSPtr());

        if (!operation->CompletedSynchronously)
        {
            innerAsyncOperation_ = operation;
        }
    }

    void ComMockReplicator::ComReplicatedOperationContext::OnDequeueComplete(
        AsyncOperationSPtr const & operation)
    {
        if (operation->CompletedSynchronously)
        {
            innerAsyncOperation_ = operation;
        }

        ErrorCode error = queueSPtr_->EndDequeue(operation, queuedOperationUPtr_);
        VERIFY_IN_THREAD(error.IsSuccess(), "OnDequeueComplete(): EndDequeue(): error = {0}", error);
        this->CompleteNow();
    }

    void ComMockReplicator::ComReplicatedOperationContext::EndDequeue(__out ComPointer<IFabricOperation> & result)
    {
        if (queuedOperationUPtr_)
        {
            HRESULT hr = queuedOperationUPtr_->Operation->QueryInterface(IID_IFabricOperation, reinterpret_cast<void**>(result.InitializationAddress()));
            VERIFY_IN_THREAD(SUCCEEDED(hr), "MockReplicationOperation does not support IFabricOperation: hr = {0}", hr);
        }
        else
        {
            Trace.WriteNoise(ReplicatedStoreTest::TestSource, "ComMockReplicator: EndDequeue = NULL");
        }
    }

    void ComMockReplicator::ComReplicatedOperationContext::Complete(bool synchronous, DWORD delayReplicateInMilliseconds)
    {
        this->completedSynchronously_ = synchronous;

        if (synchronous)
        {
            if (delayReplicateInMilliseconds > 0)
            {
                Sleep(delayReplicateInMilliseconds);
            }
            this->CompleteNow();
        }
        else
        {
            ComPointer<IFabricAsyncOperationContext> thisCPtr(this, IID_IFabricAsyncOperationContext);
            Threadpool::Post([this, thisCPtr]() { this->CompleteNow(); }, TimeSpan::FromMilliseconds(delayReplicateInMilliseconds));
        }
    }

    void ComMockReplicator::ComReplicatedOperationContext::CompleteNow()
    {
        callbackCPtr_->Invoke(this);
        callbackCPtr_.SetNoAddRef(NULL);
    }

    BOOLEAN STDMETHODCALLTYPE ComMockReplicator::ComReplicatedOperationContext::CompletedSynchronously() 
    { 
        return (innerAsyncOperation_ 
            // GetOperation()
            ? (innerAsyncOperation_->CompletedSynchronously ? TRUE : FALSE)
            // AtomicOperation()
            : (completedSynchronously_ ? TRUE : FALSE));
    }

    HRESULT ComMockReplicator::ComGetNextAsyncOperation::BeginComAsyncOperation(__in IFabricAsyncOperationCallback * callback, __out IFabricAsyncOperationContext ** operationContext)
    {
        if (callback == NULL || operationContext == NULL) { return E_POINTER; }
        return enumerator_->BeginGetNext(callback, operationContext);
    }

    HRESULT ComMockReplicator::ComGetNextAsyncOperation::EndComAsyncOperation(__in IFabricAsyncOperationContext * operationContext)
    {
        if (operationContext == NULL) { return E_POINTER; }
        return enumerator_->EndGetNext(operationContext, copyOperation_.InitializationAddress());
    }

    ErrorCode ComMockReplicator::ComGetNextAsyncOperation::End(
        AsyncOperationSPtr const & operation, 
        __out ComPointer<IFabricOperationData> & result)
    {
        auto casted = AsyncOperation::End<ComGetNextAsyncOperation>(operation);
        result.Swap(casted->copyOperation_);

        return casted->Error;
    }

    // ***********
    // ReplicaBase
    // ***********

    void ReplicaBase::Open(
        ::FABRIC_REPLICA_ROLE role, 
        ErrorCodeValue::Enum const & expectedError, 
        FABRIC_REPLICA_OPEN_MODE mode, 
        bool enableIncrementalBackup)
    {
        ::FABRIC_SERVICE_PARTITION_INFORMATION const * partitionInfo;
        HRESULT hr = partitionCPtr_->GetPartitionInfo(&partitionInfo);
        VERIFY_IS_TRUE(SUCCEEDED(hr), L"ReplicaBase::Open(): GetPartitionInfo()");

        ComPointer<IFabricStatefulServicePartition> partition(partitionCPtr_.GetRawPointer(), IID_IFabricStatefulServicePartition);

        int retries = 3;
        bool success = false;

        // Some test cases may Close() and reopen the same store to simulate recovering from crashes.
        // Secondary pump threads may take some time to complete after closing.
        //
        ErrorCode error(ErrorCodeValue::Success);
        while (retries-- > 0 && !success)
        {
            ComPointer<IFabricReplicator> replicator; 
            hr = replicatorCPtr_->QueryInterface(IID_IFabricReplicator, replicator.VoidInitializationAddress());
            VERIFY(SUCCEEDED(hr), L"QueryInterface(IID_IFabricReplicator): {0}", hr);

            replicatedStoreSettings_.CommitBatchingPeriod = 100;

            // Ensure secondary processes notification before replying to primary or notification
            // verification logic could fail when it races with an OnReplicationOperation callback
            //
            replicatedStoreSettings_.NotificationMode = SecondaryNotificationMode::BlockSecondaryAck;

            storeUPtr_ = EseReplicatedStore::Create(
                Common::Guid(Common::ServiceInfoUtility::GetPartitionId(partitionInfo)),
                replicaId_, 
                move(replicator),
                replicatedStoreSettings_,
                EseLocalStoreSettings(
                    databaseFilename_, 
                    databaseDirectory_,
                    enableIncrementalBackup),
                Api::IStoreEventHandlerPtr(),
                Api::ISecondaryEventHandlerPtr(this, this->CreateComponentRoot()),
                *this);

            AutoResetEvent waiter(false);
            AsyncOperationSPtr operation = Store.BeginOpen(
                mode,
                partition, 
                [this, &waiter](AsyncOperationSPtr const &) { waiter.Set(); },
                this->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"ReplicaBase::Open(): Store.Open() timeout");

            ComPointer<IFabricReplicator> unused;
            error = Store.EndOpen(operation, unused);

            success = error.IsError(expectedError);

            if (!success)
            {
                Trace.WriteWarning(
                    ReplicatedStoreTest::TestSource, 
                    "{0} open failed due to {1}. Expecting {2}. Retrying.", 
                    Store.TraceId, 
                    error, 
                    expectedError);

                storeUPtr_.reset();
                Sleep(500);
            }
        }
        VERIFY_IS_TRUE(success, L"ReplicaBase::Open(): Store.Open()");

        if (error.IsSuccess() && role != ::FABRIC_REPLICA_ROLE_NONE)
        {
            ChangeRole(role);
        }
    }

    void ReplicaBase::ChangeRole(::FABRIC_REPLICA_ROLE role)
    {
        if (Store.ReplicaRole != role)
        {
            if (role == ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || role == FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
            {
                copyNotificationProcessed_.Reset();
                replNotificationProcessed_.Reset();
            }

            AutoResetEvent waiter(false);
            auto operation = Store.BeginChangeRole(
                role,
                [this, &waiter](AsyncOperationSPtr const &) { waiter.Set(); },
                this->CreateAsyncOperationRoot());

            VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"ReplicaBase::ChangeRole(): timeout");

            wstring unused;
            ErrorCode error = Store.EndChangeRole(operation, unused);
            VERIFY_IS_TRUE(error.IsSuccess(), L"ReplicaBase::ChangeRole(): Store.ChangeRole()");
        }
    }

    void ReplicaBase::UpdateEpoch(LONGLONG configurationNumber, LONGLONG datalossNumber, ::FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        ::FABRIC_EPOCH epoch;
        epoch.ConfigurationNumber = configurationNumber;
        epoch.DataLossNumber = datalossNumber;
        epoch.Reserved = NULL;

        AutoResetEvent waiter(false);
        ErrorCode error;
        do
        {
            waiter.Reset();
            auto operation = Store.BeginUpdateEpoch(epoch, sequenceNumber,
               [this, &waiter](AsyncOperationSPtr const &)
               {
            	   waiter.Set();
               },
               this->CreateAsyncOperationRoot());

               VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"ReplicaBase::UpdateEpoch(): timeout");
               error = Store.EndUpdateEpoch(operation);
        } while(error.IsError(ErrorCodeValue::StoreWriteConflict));

        VERIFY_IS_TRUE(error.IsSuccess(), L"ReplicaBase::UpdateEpoch(): Store.UpdateEpoch()");
    }

    void ReplicaBase::Close(ErrorCodeValue::Enum allowedError)
    {
        AutoResetEvent waiter(false);
        auto operation = Store.BeginClose(
            [this, &waiter](AsyncOperationSPtr const &) { waiter.Set(); },
            this->CreateAsyncOperationRoot());

        VERIFY_IS_TRUE(waiter.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"ReplicaBase::Close(): timeout");
        ErrorCode error = Store.EndClose(operation);
        VERIFY_IS_TRUE(error.IsSuccess() || error.IsError(allowedError), L"ReplicaBase::Close(): Store.Close()");
    }

    void ReplicaBase::Abort()
    {   
        Store.Abort();
    }

    void ReplicaBase::AsyncAbort()
    {   
        Store.BeginClose(
            [this](AsyncOperationSPtr const & operation) { Store.EndClose(operation).ReadValue(); },
            this->CreateAsyncOperationRoot());

        Store.Test_TerminateLocalStore();

        Sleep(1000);
    }

    void ReplicaBase::SetAccessDenied()
    {
        partitionCPtr_->SetAccessDenied();
    }

    vector<ReplicaBase::DataItem> ReplicaBase::ReadUserData(
        _int64 startOperationLSN,
        _int64 endOperationLSN)
    {
        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "{0} reading user data: range = [{1}, {2}]", 
            Store.TraceId,
            startOperationLSN,
            endOperationLSN);

        // Read via ReplicatedStore::Transaction to cover read-only transactions
        TransactionSPtr txSPtr;

#if defined(PLATFORM_UNIX)
        int timeout = 50;
        while (!Store.Test_IsSecondaryPumpClosed() && timeout--) Sleep(10);
#endif
        ErrorCode error = Store.LocalStore->CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        IStoreBase::EnumerationSPtr enumSPtr;
        error = Store.LocalStore->CreateEnumerationByOperationLSN(txSPtr, startOperationLSN, enumSPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        vector<DataItem> dataItems;
        while ((error = enumSPtr->MoveNext()).IsSuccess())
        {
            wstring type;
            wstring key;
            vector<byte> data;
            FILETIME lastModifiedOnPrimary;

            enumSPtr->CurrentType(type);
            enumSPtr->CurrentKey(key);
            enumSPtr->CurrentValue(data);
            enumSPtr->CurrentLastModifiedOnPrimaryFILETIME(lastModifiedOnPrimary);

            ::FABRIC_SEQUENCE_NUMBER operationLSN = 0;
            enumSPtr->CurrentOperationLSN(operationLSN);

            if (operationLSN > endOperationLSN)
            {
                error = ErrorCodeValue::EnumerationCompleted;
                break;
            }

            // Progress data is not part of the user data
            // and is verified separately
            //
            if (type != Constants::ProgressDataType &&
                type != Constants::FabricTimeDataType &&
                type != Constants::LocalStoreIncrementalBackupDataType)
            {
                dataItems.push_back(DataItem(type, key, operationLSN, data, lastModifiedOnPrimary));
            }

            Trace.WriteNoise(
                ReplicatedStoreTest::TestSource, 
                "{0} read data item: [{1}, {2}, {3}, {4}]", 
                Store.TraceId, 
                type, 
                key, 
                operationLSN, 
                data.size());
        }

        VERIFY(error.IsError(ErrorCodeValue::EnumerationCompleted), "{0} couldn't read all data items. error = {1}", Store.TraceId, error);

        error = txSPtr->Commit();
        VERIFY(error.IsSuccess(), "{0} ReadUserData: commit failed. error = {1}", error);

        return dataItems;
    }

    vector<ReplicaBase::DataItem> ReplicaBase::GetNotificationData(
        _int64 startOperationLSN, 
        _int64 endOperationLSN)
    {
        VERIFY_IS_TRUE(copyNotificationProcessed_.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"copyNotificationProcessed_.WaitOne() timeout");

        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "GetNotificationData: copy = {0} replicator = {1}",
            copyNotificationLsn_,
            replicatorCPtr_->LastNotificationLSN);

        // Secondary may be uptodate after copy. If not, then wait for all replication notifications to process
        //
        if (copyNotificationLsn_ < replicatorCPtr_->LastNotificationLSN)
        {
            VERIFY_IS_TRUE(replNotificationProcessed_.WaitOne(TimeSpan::FromSeconds(WaitOneTimeoutInSeconds)), L"replNotificationProcessed_.WaitOne() timeout");
        }

        vector<DataItem> items = notificationItems_;

        items.erase(std::remove_if(items.begin(), items.end(),
            [&](DataItem const & it) -> bool { return (it.OperationLSN < startOperationLSN || it.OperationLSN > endOperationLSN); }),
            items.end());

        SortData(items);

        return move(items);
    }

    vector<ReplicaBase::DataItem> ReplicaBase::GetNotificationDataToVerify(vector<DataItem> const & dataToVerify)
    {
        vector<DataItem> items = dataToVerify;

        // remove tombstones for comparison with notification data
        items.erase(std::remove_if(items.begin(), items.end(),
            [&](DataItem const & it) -> bool { return ReplicatedStore::IsStoreMetadataType(it.Type); }),
            items.end());

        SortData(items);

        return move(items);
    }

    void ReplicaBase::SortData(vector<DataItem> & dataToSort)
    {
        std::sort(dataToSort.begin(), dataToSort.end(),
            [](DataItem const & left, DataItem const & right) -> bool
            {
                if (StringUtility::IsLessCaseInsensitive(left.Type, right.Type)) 
                { 
                    return true; 
                } 
                else if (left.Type == right.Type && StringUtility::IsLessCaseInsensitive(left.Key, right.Key)) 
                { 
                    return true;
                }

                return false;
            });
    }

    void ReplicaBase::VerifyData(
        vector<DataItem> const & dataToVerify, 
        _int64 startOperationLSN,
        _int64 endOperationLSN)
    {
        // verify data from secondary notifications
        //
        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource,
            "verifying notification contents");

        vector<DataItem> fromNotifications = GetNotificationData(startOperationLSN, endOperationLSN);
        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource,
            "verifying notification contents {0} {1}", dataToVerify.size(), fromNotifications.size());

        vector<DataItem> notificationDataToVerify = GetNotificationDataToVerify(dataToVerify);

        VerifyEqualData(fromNotifications, notificationDataToVerify, startOperationLSN, endOperationLSN);

        // verify data persisted to database
        //
        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "verifying database contents");

        vector<DataItem> fromDatabase = ReadUserData(startOperationLSN, endOperationLSN);
        VerifyEqualData(fromDatabase, dataToVerify, startOperationLSN, endOperationLSN);
    }

    void ReplicaBase::VerifyEqualData(
        vector<DataItem> const & leftData, 
        vector<DataItem> const & rightData,
        _int64 startOperationLSN,
        _int64 endOperationLSN)
    { 
        size_t minSize = leftData.size();
        if (minSize > rightData.size())
        {
            minSize = rightData.size();
        }
        vector<size_t> mismatchIndexes;

        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "{0} verifying data: leftData size = {1} rightData size = {2} range = [{3}, {4}]", 
            Store.TraceId, 
            leftData.size(), 
            rightData.size(),
            startOperationLSN,
            endOperationLSN);

        for (size_t ix = 0; ix < minSize; ++ix)
        {
            bool match = true;

            if (leftData[ix].Type != rightData[ix].Type || 
                leftData[ix].Key != rightData[ix].Key || 
                leftData[ix].OperationLSN != rightData[ix].OperationLSN || 
                leftData[ix].Bytes.size() != rightData[ix].Bytes.size())
            {
                match = false;
            }

#if !defined(PLATFORM_UNIX)

            if (leftData[ix].LastModifiedOnPrimaryInt64 != rightData[ix].LastModifiedOnPrimaryInt64)
            {
                match = false;
            }
#endif

            if (match == true)
            {
                for (size_t jx = 0; jx < leftData[ix].Bytes.size(); ++jx)
                {
                    if (leftData[ix].Bytes[jx] != rightData[ix].Bytes[jx])
                    {
                        match = false;
                        break;
                    }
                } 
            }

            if (!match)
            {
                mismatchIndexes.push_back(ix);
            }
            else
            {
#if !defined(PLATFORM_UNIX)
				Trace.WriteNoise(
                    ReplicatedStoreTest::TestSource, 
                    "VerifyData(): ({0}) [{1}, {2}, {3}, {4}, {5}]", 
                    ix,
                    leftData[ix].Type, 
                    leftData[ix].Key, 
                    leftData[ix].OperationLSN,
                    leftData[ix].Bytes.size(),
                    leftData[ix].LastModifiedOnPrimaryInt64);
#else
				Trace.WriteNoise(
                    ReplicatedStoreTest::TestSource, 
                    "VerifyData(): ({0}) [{1}, {2}, {3}, {4}]", 
                    ix,
                    leftData[ix].Type, 
                    leftData[ix].Key, 
                    leftData[ix].OperationLSN,
                    leftData[ix].Bytes.size());        
#endif
            }
        }

        // Debug the mismatching data items if any
        for (size_t ix = 0; ix < mismatchIndexes.size(); ++ix)
        {
            size_t index = mismatchIndexes[ix];
#if !defined(PLATFORM_UNIX)
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): mismatch({0}): leftData [{1}, {2}, {3}, {4}, {5}]", 
                index,
                leftData[index].Type,
                leftData[index].Key,
                leftData[index].OperationLSN,
                leftData[index].Bytes.size(),
                leftData[index].LastModifiedOnPrimaryInt64);

            Trace.WriteError(
                ReplicatedStoreTest::TestSource,
                "!VerifyData(): mismatch({0}): rightData [{1}, {2}, {3}, {4}, {5}]",
                index,
                rightData[index].Type,
                rightData[index].Key,
                rightData[index].OperationLSN,
                rightData[index].Bytes.size(),
                rightData[index].LastModifiedOnPrimaryInt64);
#else
			Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): mismatch({0}): leftData [{1}, {2}, {3}, {4}]", 
                index,
                leftData[index].Type,
                leftData[index].Key,
                leftData[index].OperationLSN,
                leftData[index].Bytes.size());

            Trace.WriteError(
                ReplicatedStoreTest::TestSource,
                "!VerifyData(): mismatch({0}): rightData [{1}, {2}, {3}, {4}]",
                index,
                rightData[index].Type,
                rightData[index].Key,
                rightData[index].OperationLSN,
                rightData[index].Bytes.size());
#endif
        }

        // Debug extra leftData items if any
        for (size_t ix = minSize; ix < leftData.size(); ++ix)
        {
#if !defined(PLATFORM_UNIX)
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): extra leftData({0}): [{1}, {2}, {3}, {4}, {5}]",
                ix,
                leftData[ix].Type,
                leftData[ix].Key,
                leftData[ix].OperationLSN,
                leftData[ix].Bytes.size(),
                leftData[ix].LastModifiedOnPrimaryInt64);
  
#else
			Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): extra leftData({0}): [{1}, {2}, {3}, {4}]",
                ix,
                leftData[ix].Type,
                leftData[ix].Key,
                leftData[ix].OperationLSN,
                leftData[ix].Bytes.size());
#endif
        }

        // Debug extra rightData items if any
        for (size_t ix = minSize; ix < rightData.size(); ++ix)
        {
#if !defined(PLATFORM_UNIX)
			Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): extra rightData({0}): [{1}, {2}, {3}, {4}, {5}]",
                ix,
                rightData[ix].Type,
                rightData[ix].Key,
                rightData[ix].OperationLSN,
                rightData[ix].Bytes.size(),
                rightData[ix].LastModifiedOnPrimaryInt64);
#else
			Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyData(): extra rightData({0}): [{1}, {2}, {3}, {4}]",
                ix,
                rightData[ix].Type,
                rightData[ix].Key,
                rightData[ix].OperationLSN,
                rightData[ix].Bytes.size());
#endif
        }

        bool success = (mismatchIndexes.size() == 0 && leftData.size() == rightData.size());
        VERIFY_IS_TRUE(success);
    }

    _int64 ReplicaBase::ReadProgress()
    {
        ::FABRIC_SEQUENCE_NUMBER sequenceNumber;
        ErrorCode error = Store.GetLastCommittedSequenceNumber(sequenceNumber);
        VERIFY_IS_TRUE(error.IsSuccess(), L"ReadProgress(): Store.GetLastCommittedSequenceNumber()");

        return sequenceNumber;
    }

    vector<ProgressVectorEntry> ReplicaBase::ReadProgressVector(bool includeCurrentEpoch)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "{0} reading progress vector: includeCurrentEpoch={1}", Store.TraceId, includeCurrentEpoch);

        // Read via ReplicatedStore::Transaction to cover read-only transactions
        TransactionSPtr txSPtr;

        ErrorCode error = Store.LocalStore->CreateTransaction(txSPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        IStoreBase::EnumerationSPtr enumSPtr;
        error = Store.LocalStore->CreateEnumerationByTypeAndKey(txSPtr, Constants::ProgressDataType, Constants::EpochHistoryDataName, enumSPtr);
        VERIFY_IS_TRUE(error.IsSuccess());

        vector<ProgressVectorEntry> epochHistory;
        if (enumSPtr->MoveNext().IsSuccess())
        {
            wstring type;
            wstring key;
            vector<byte> data;

            enumSPtr->CurrentType(type);
            enumSPtr->CurrentKey(key);
            enumSPtr->CurrentValue(data);

            if (key == Constants::EpochHistoryDataName)
            {
                ProgressVectorData epochHistoryData;

                VERIFY_IS_TRUE(FabricSerializer::Deserialize(epochHistoryData, data).IsSuccess());

                epochHistory.swap(epochHistoryData.ProgressVector);

                for (size_t ix = 0; ix < epochHistory.size(); ++ix)
                {
                    Trace.WriteNoise(
                        ReplicatedStoreTest::TestSource, 
                        "{0} read epoch history item: [{1}.{2:X}, {3}]", 
                        Store.TraceId, 
                        epochHistory[ix].Epoch.DataLossNumber,
                        epochHistory[ix].Epoch.ConfigurationNumber,
                        epochHistory[ix].LastOperationLSN);
                }
            }
        }

        if (includeCurrentEpoch)
        {
            error = Store.LocalStore->CreateEnumerationByTypeAndKey(txSPtr, Constants::ProgressDataType, Constants::CurrentEpochDataName, enumSPtr);
            VERIFY_IS_TRUE(error.IsSuccess());

            if (enumSPtr->MoveNext().IsSuccess())
            {
                wstring type;
                wstring key;
                vector<byte> data;

                enumSPtr->CurrentType(type);
                enumSPtr->CurrentKey(key);
                enumSPtr->CurrentValue(data);

                if (key == Constants::CurrentEpochDataName)
                {
                    ::FABRIC_EPOCH epoch;
                    CurrentEpochData currentEpoch;

                    VERIFY_IS_TRUE(FabricSerializer::Deserialize(currentEpoch, data).IsSuccess());

                    epoch = currentEpoch.Epoch;

                    // sequence number for the current epoch doesn't matter
                    epochHistory.push_back(ProgressVectorEntry(epoch, 0));

                    Trace.WriteNoise(
                        ReplicatedStoreTest::TestSource, 
                        "{0} read current epoch: [{1}.{2:X}, {3}]", 
                        Store.TraceId, 
                        epoch.DataLossNumber,
                        epoch.ConfigurationNumber,
                        0);
                }
            }
        }

        error = txSPtr->Commit();
        VERIFY(error.IsSuccess(), "{0} ReadProgressVector: commit failed. error = {1}", error);

        // Cannot change roles until pending transactions are complete
        enumSPtr.reset();
        txSPtr.reset();

        return epochHistory;
    }

    void ReplicaBase::VerifyProgress(_int64 expectedProgress)
    {
        int retryCount = 5;
        FABRIC_SEQUENCE_NUMBER sequenceNumber = 0;

        while (retryCount-- > 0)
        {
            sequenceNumber = ReadProgress();

            if (sequenceNumber == expectedProgress) 
            {
                break;
            }
            else
            {
                Trace.WriteError(
                    ReplicatedStoreTest::TestSource, 
                    "!VerifyProgress(): expected = {0} read = {1}",
                    expectedProgress,
                    sequenceNumber);

                Sleep(1000);
            }
        }

        bool success = (sequenceNumber == expectedProgress);
        VERIFY_IS_TRUE(success);
    }

    long ReplicaBase::GetFaultCount()
    {
        return partitionCPtr_->GetFaultCount();
    }

    void ReplicaBase::VerifyFaultCount()
    {
        this->VerifyFaultCount(0);
    }

    void ReplicaBase::VerifyFaultCount(long expectedCount)
    {
        auto faultCount = partitionCPtr_->GetFaultCount();

        bool success = (faultCount == expectedCount);

        if (!success)
        {
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyFaultCount(): expected = {0} count = {1}",
                expectedCount,
                faultCount);
        }

        VERIFY_IS_TRUE(success);
    }


    void ReplicaBase::VerifyCopyCounts(
        uint64 expectedFullCopyCount, 
        uint64 expectedPartialCopyCount)
    {
        uint64 actualFullCopyCount = storeUPtr_->CopyStatistics.GetFullCopyCount();
        uint64 actualPartialCopyCount = storeUPtr_->CopyStatistics.GetPartialCopyCount();

        bool success = (actualFullCopyCount == expectedFullCopyCount && actualPartialCopyCount == expectedPartialCopyCount);

        if (!success)
        {
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyCopyCounts(): full [expected={0} actual={1}] partial [expected={2} actual={3}]",
                expectedFullCopyCount,
                actualFullCopyCount,
                expectedPartialCopyCount,
                actualPartialCopyCount);
        }

        VERIFY_IS_TRUE(success);
    }

    FABRIC_SEQUENCE_NUMBER ReplicaBase::GetTombstoneLowWatermark()
    {
        TombstoneLowWatermarkData lowWatermark;
        auto error = storeUPtr_->GetTombstoneLowWatermark(lowWatermark);
        VERIFY_IS_TRUE(error.IsSuccess());

        return lowWatermark.OperationLSN;
    }

    void ReplicaBase::VerifyTombstoneLowWatermark(FABRIC_SEQUENCE_NUMBER toVerify)
    {
        auto lowWatermark = this->GetTombstoneLowWatermark();
        VERIFY(lowWatermark == toVerify, "Low Watermarks: LSN={0} expected={1}", lowWatermark, toVerify);
    }

    void ReplicaBase::VerifyTombstones1(vector<DataItem> const & dataItems, size_t expectedTombstoneCount)
    {
        VerifyTombstones(dataItems, expectedTombstoneCount, expectedTombstoneCount, true);
    }

    void ReplicaBase::VerifyTombstones2(vector<DataItem> const & dataItems, size_t expectedTombstoneCount)
    {
        VerifyTombstones(dataItems, expectedTombstoneCount, expectedTombstoneCount, false);
    }

    void ReplicaBase::VerifyTombstones2(vector<DataItem> const & dataItems, size_t expectedTombstoneCount, size_t expectedEstimatedSize)
    {
        VerifyTombstones(dataItems, expectedTombstoneCount, expectedEstimatedSize, false);
    }

    void ReplicaBase::VerifyTombstones(
        vector<DataItem> const & dataItems, 
        size_t expectedTombstoneCount,
        size_t expectedEstimatedSize,
        bool expectVersion1)
    {
        size_t tombstoneCount = 0;
        for (auto const & item : dataItems)
        {
            if (item.Type == Constants::TombstoneDataType)
            {
                if (expectVersion1)
                {
                    wstring type;
                    wstring key;
                    bool success = storeUPtr_->TryParseTombstoneKey1(item.Key, type, key);
                    VERIFY(success, "Parse version1 tombstone key: '{0}'", item.Key);
                    VERIFY(item.Bytes.size() == 1, "Version1 tombstone value size: {0}", item.Bytes.size());
                }
                else
                {
                    wstring type;
                    wstring key;
                    bool success = storeUPtr_->TryParseTombstoneKey1(item.Key, type, key);
                    VERIFY(!success, "Parse version2 tombstone key: '{0}'", item.Key);

                    TombstoneData tombstone;
                    auto error = FabricSerializer::Deserialize(tombstone, const_cast<vector<byte>&>(item.Bytes));
                    VERIFY(error.IsSuccess(), "Deserialize version 2 tombstone: '{0}'", item.Key);
                    
                    TombstoneData tombstoneToVerify(tombstone.LiveEntryType, tombstone.LiveEntryKey, item.OperationLSN, tombstone.Index);
                    VERIFY(tombstoneToVerify.Key == item.Key, "tombstone.Key='{0}' dataItem.Key='{1}'", tombstone.Key, item.Key);
                }

                ++tombstoneCount;
            }
        }

        VERIFY(tombstoneCount == expectedTombstoneCount, "tombstones={0} expected={1}", tombstoneCount, expectedTombstoneCount);

        auto estimatedCount = storeUPtr_->Test_GetTombstoneCount();
        VERIFY(estimatedCount == expectedEstimatedSize, "estimated={0} expected={1}", estimatedCount, expectedTombstoneCount);
    }

    void ReplicaBase::VerifyTombstoneOrder(
        vector<DataItem> const & dataItems, 
        FABRIC_SEQUENCE_NUMBER startingLsn, 
        size_t expectedSize)
    {
        FABRIC_SEQUENCE_NUMBER expectedLsn = startingLsn;

        for (auto const & item : dataItems)
        {
            if (!StoreConfig::GetConfig().EnableTombstoneCleanup2)
            {
                wstring type;
                wstring key;
                bool success = storeUPtr_->TryParseTombstoneKey1(item.Key, type, key);
                VERIFY(success, "Parse version1 tombstone key: '{0}'", item.Key);
                VERIFY(item.Bytes.size() == 1, "Version1 tombstone value size: {0}", item.Bytes.size());
            }
            else
            {
                wstring type;
                wstring key;
                bool success = storeUPtr_->TryParseTombstoneKey1(item.Key, type, key);
                VERIFY(!success, "Parse version2 tombstone key: '{0}'", item.Key);

                TombstoneData tombstone;
                auto error = FabricSerializer::Deserialize(tombstone, const_cast<vector<byte>&>(item.Bytes));
                VERIFY(error.IsSuccess(), "Deserialize version 2 tombstone: '{0}'", item.Key);
                
                TombstoneData tombstoneToVerify(tombstone.LiveEntryType, tombstone.LiveEntryKey, item.OperationLSN, tombstone.Index);
                VERIFY(tombstoneToVerify.Key == item.Key, "tombstone.Key='{0}' dataItem.Key='{1}'", tombstone.Key, item.Key);
            }

            VERIFY(item.OperationLSN == expectedLsn, "LSN mismatch: data={0} expected={1}", item.OperationLSN, expectedLsn);

            expectedLsn += 2; // skip 1 LSN for insert before delete
        }

        VERIFY(dataItems.size() == expectedSize, "tombstone count mismatch: count={0} expected={1}", dataItems.size(), expectedSize);
    }

    vector<ReplicaBase::DataItem> ReplicaBase::WaitForDataCount(size_t expectedCount)
    {
        int retryCount = 10;
        auto data = this->ReadUserData();
        while (data.size() > expectedCount && retryCount-- > 0)
        {
            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource, 
                "{0} waiting for data size: count={1} expected={2}", 
                Store.TraceId,
                data.size(), 
                expectedCount);

            Sleep(500);

            data = this->ReadUserData();
        }
        VERIFY(data.size() == expectedCount, "data size mismatch: count={0} expected={1}", data.size(), expectedCount);

        return data;
    }

    void ReplicaBase::VerifyProgressVector(vector<ProgressVectorEntry> const & progressVectorToVerify, bool includeCurrentEpoch)
    {
        vector<ProgressVectorEntry> progressVector = ReadProgressVector(includeCurrentEpoch);

        vector<size_t> mismatchIndexes;
        size_t minSize = progressVector.size();
        if (progressVectorToVerify.size() < minSize)
        {
            minSize = progressVectorToVerify.size();
        }

        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "{0} verifying progress vector: vector size = {1} vectorToVerify size = {2}",
            Store.TraceId, 
            progressVector.size(), 
            progressVectorToVerify.size());

        for (size_t ix = 0; ix < progressVector.size(); ++ix)
        {
            bool match = true;

            if (progressVector[ix].Epoch.ConfigurationNumber != progressVectorToVerify[ix].Epoch.ConfigurationNumber
                || progressVector[ix].Epoch.DataLossNumber != progressVectorToVerify[ix].Epoch.DataLossNumber)
            {
                match = false;
            }

            // Sequence number is irrelevant for the last item (current epoch)
            if (match && ix < progressVector.size() - 1)
            {
                if (progressVector[ix].LastOperationLSN != progressVectorToVerify[ix].LastOperationLSN)
                {
                    match = false;
                }
            }

            if (!match)
            {
                mismatchIndexes.push_back(ix);
            }
        }

        for (size_t ix = 0; ix < mismatchIndexes.size(); ++ix)
        {
            size_t index = mismatchIndexes[ix];

            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyProgressVector(): mismatch({0}): [{1}.{2:X}, {3}] expected=[{4}.{5:X}, {6}]", 
                index,
                progressVector[index].Epoch.DataLossNumber,
                progressVector[index].Epoch.ConfigurationNumber,
                progressVector[index].LastOperationLSN,
                progressVectorToVerify[index].Epoch.DataLossNumber,
                progressVectorToVerify[index].Epoch.ConfigurationNumber,
                progressVectorToVerify[index].LastOperationLSN);
        }

        for (size_t ix = minSize; ix < progressVector.size(); ++ix)
        {
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyProgressVector(): extra progress vector entry({0}): [{1}.{2:X}, {3}]",
                ix,
                progressVector[ix].Epoch.DataLossNumber,
                progressVector[ix].Epoch.ConfigurationNumber,
                progressVector[ix].LastOperationLSN);
        }

        for (size_t ix = minSize; ix < progressVectorToVerify.size(); ++ix)
        {
            Trace.WriteError(
                ReplicatedStoreTest::TestSource, 
                "!VerifyProgressVector(): extra progress vector to verify entry({0}): [{1}.{2:X}, {3}]",
                ix,
                progressVectorToVerify[ix].Epoch.DataLossNumber,
                progressVectorToVerify[ix].Epoch.ConfigurationNumber,
                progressVectorToVerify[ix].LastOperationLSN);
        }

        bool success = (mismatchIndexes.size() == 0 && progressVector.size() == progressVectorToVerify.size());
        VERIFY_IS_TRUE(success);
    }

    void ReplicaBase::InsertData(int typeCount, int keyCount, int bytesCount, int targetModulo, wstring const & typePrefix, wstring const & keyPrefix, ErrorCodeValue::Enum retryableError)
    {
        WriteData(typeCount, keyCount, bytesCount, targetModulo, typePrefix, keyPrefix, ReplicationOperationType::Insert, retryableError);
    }

    void ReplicaBase::UpdateData(int typeCount, int keyCount, int bytesCount, int targetModulo, wstring const & typePrefix, wstring const & keyPrefix)
    {
        WriteData(typeCount, keyCount, bytesCount, targetModulo, typePrefix, keyPrefix, ReplicationOperationType::Update);
    }

    void ReplicaBase::DeleteData(int typeCount, int keyCount, int targetModulo, wstring const & typePrefix, wstring const & keyPrefix, ErrorCodeValue::Enum retryableError)
    {
        WriteData(typeCount, keyCount, 0, targetModulo, typePrefix, keyPrefix, ReplicationOperationType::Delete, retryableError);
    }

    void ReplicaBase::WriteData(
        int typeCount, 
        int keyCount, 
        int bytesCount, 
        int targetModulo,
        wstring const & typePrefix,
        wstring const & keyPrefix,
        ReplicationOperationType::Enum operation,
        ErrorCodeValue::Enum retryableError)
    {
        TransactionSPtr sptr(nullptr);
        this->InnerWriteData(
            sptr,
            typeCount,
            keyCount,
            bytesCount,
            targetModulo,
            typePrefix,
            keyPrefix,
            operation,
            retryableError);
    }

    void ReplicaBase::InnerWriteData(
        __in TransactionSPtr & txSPtr,
        int typeCount, 
        int keyCount, 
        int bytesCount, 
        int targetModulo,
        wstring const & typePrefix,
        wstring const & keyPrefix,
        ReplicationOperationType::Enum operation,
        ErrorCodeValue::Enum retryableError)
    {
        bool createTx = (txSPtr.get() == nullptr);
        bool doneWrite = false;
        bool retryWrite = false;

        while (!doneWrite)
        {
            retryWrite = false;

            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource, 
                "{0} writing data({1}, {2}, {3}, {4}, {5}, {6}, {7})", 
                Store.TraceId,
                typeCount,
                keyCount,
                bytesCount,
                targetModulo,
                typePrefix,
                keyPrefix,
                operation);

            if (createTx)
            { 
                auto error = Store.CreateTransaction(txSPtr);
                VERIFY(error.IsSuccess(), "CreateTransaction: error={0}", error);
            }

            for (int ix = 0; ix < typeCount && !retryWrite; ++ix)
            {
                wstring type = wformatString("{0}{1}", typePrefix, ix);

                // Can be multi-threaded in certain test cases (SerialCommit)
                {
                    AcquireWriteLock lock(ReplicatedStoreTest::KnownDataTypesLock);

                    ReplicatedStoreTest::KnownDataTypes.insert(type);
                }

                for (int jx = 0; jx < keyCount && !retryWrite; ++jx)
                {
                    if ((ix + jx) % targetModulo != 0)
                    {
                        continue;
                    }

                    wstring key = wformatString("{0}{1}", keyPrefix, jx);

                    vector<byte> bytes;
                    if (operation != ReplicationOperationType::Delete)
                    {
                        for (int kx = 0; kx < bytesCount; ++kx)
                        {
                            if (operation == ReplicationOperationType::Insert)
                            {
                                bytes.push_back(static_cast<byte>(ix + jx + kx));
                            }
                            else
                            {
                                bytes.push_back(static_cast<byte>(-ix - jx - kx));
                            }
                        }
                    }

                    ErrorCode error;

                    switch (operation)
                    {
                    case ReplicationOperationType::Insert:
                        error = Store.Insert(
                            txSPtr, 
                            type, 
                            key, 
                            bytes.data(), 
                            bytes.size());
                        break;

                    case ReplicationOperationType::Update:
                        error = Store.Update(
                            txSPtr, 
                            type, 
                            key, 
                            0,
                            key,
                            bytes.data(), 
                            bytes.size());
                        break;

                    case ReplicationOperationType::Delete:
                        error = Store.Delete(
                            txSPtr, 
                            type, 
                            key);
                        break;

                    default:
                        Assert::CodingError("{0} invalid operation {1}", Store.TraceId, operation);
                    } // end switch operation type

#if defined(PLATFORM_UNIX)
                    if (retryableError != ErrorCodeValue::Success && (error.IsError(retryableError) || error.IsError(ErrorCodeValue::StoreRecordNotFound)))
#else
                    if (retryableError != ErrorCodeValue::Success && error.IsError(retryableError))
#endif
                    {
                        doneWrite = false;
                        retryWrite = true;
                    }
                    else
                    {
                        doneWrite = true;
                        VERIFY(error.IsSuccess(), "{0} ReplicaBase::WriteData(): error = {1}", Store.TraceId, error);
                    }
                } // end for keys
            } // end for types

            if (retryWrite)
            {
                Sleep(10);
            }
            else if (createTx)
            {
                doneWrite = true;
                auto error = txSPtr->Commit();
#if defined(PLATFORM_UNIX)
                if (retryableError != ErrorCodeValue::Success && error.IsError(retryableError))
                {
                    Sleep(10);
                }
                else
#endif
                VERIFY(error.IsSuccess(), "{0} ReplicaBase::WriteData(): Commit(): error = {1}", Store.TraceId, error);
            }
        }
    }

    ErrorCode ReplicaBase::OnCopyComplete(Api::IStoreEnumeratorPtr const & enumeratorPtr)
    {
        notificationItems_.clear();
        
        std::set<wstring> dataTypes;
        {
            AcquireReadLock lock(ReplicatedStoreTest::KnownDataTypesLock);

            dataTypes = ReplicatedStoreTest::KnownDataTypes;
        }

        Trace.WriteInfo(
            ReplicatedStoreTest::TestSource, 
            "OnCopyComplete(): types = {0}",
            dataTypes);

        if (copyNotificationDelay_ > TimeSpan::Zero)
        {
            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource, 
                "Injecting OnCopyComplete() delay {0}",
                copyNotificationDelay_);

            Sleep(static_cast<DWORD>(copyNotificationDelay_.TotalPositiveMilliseconds()));
        }

        FABRIC_SEQUENCE_NUMBER highestCopyLsn = 0;

        for (auto it = dataTypes.begin(); it != dataTypes.end(); ++it)
        {

            vector<DataItem> metadata;
            {
                Api::IStoreItemMetadataEnumeratorPtr enumPtr;
                auto error = enumeratorPtr->EnumerateMetadata(*it, L"", false, enumPtr);
                VERIFY(error.IsSuccess(), "EnumerateMetadata(): {0}", error);

                while ((error = enumPtr->MoveNext()).IsSuccess())
                {
                    Api::IStoreItemMetadataPtr metadataPtr = enumPtr->GetCurrentItemMetadata();

                    metadata.push_back(DataItem(
                        metadataPtr->GetType(),
                        metadataPtr->GetKey(),
                        metadataPtr->GetOperationLSN(),
                        vector<byte>(),
                        metadataPtr->GetLastModifiedOnPrimaryUtc().AsFileTime));
                }
                
                if (copyNotificationDelay_ > TimeSpan::Zero)
                {
                    VERIFY(error.IsError(ErrorCodeValue::ObjectClosed), "OnCopyComplete-metadata: {0}", error);

                    return error;
                }
                else
                {
                    VERIFY(error.IsError(ErrorCodeValue::EnumerationCompleted), "OnCopyComplete-metadata: {0}", error);
                }
            }

            vector<DataItem> items;
            vector<DataItem> itemsMetadata;
            {
                Api::IStoreItemEnumeratorPtr enumPtr;
                auto error = enumeratorPtr->Enumerate(*it, L"", false, enumPtr);
                VERIFY_IS_TRUE(error.IsSuccess(), L"Enumerate()");

                while ((error = enumPtr->MoveNext()).IsSuccess())
                {
                    Api::IStoreItemPtr itemPtr = enumPtr->GetCurrentItem();

                    items.push_back(DataItem(
                        itemPtr->GetType(),
                        itemPtr->GetKey(),
                        itemPtr->GetOperationLSN(),
                        itemPtr->GetValue(),
                        itemPtr->GetLastModifiedOnPrimaryUtc().AsFileTime));

                    itemsMetadata.push_back(DataItem(
                        itemPtr->GetType(),
                        itemPtr->GetKey(),
                        itemPtr->GetOperationLSN(),
                        vector<byte>(),
                        itemPtr->GetLastModifiedOnPrimaryUtc().AsFileTime));

                    if (highestCopyLsn < itemPtr->GetOperationLSN())
                    {
                        highestCopyLsn = itemPtr->GetOperationLSN();
                    }
                }
                VERIFY(error.IsError(ErrorCodeValue::EnumerationCompleted), "OnCopyComplete-data: {0}", error);
            }

            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource, 
                "verifying metadata/data enumeration for type '{0}': highest LSN = {1}",
                *it,
                highestCopyLsn);

            // verify enumeration of metadata matches items
            VerifyEqualData(metadata, itemsMetadata, 0, numeric_limits<_int64>::max());

            notificationItems_.insert(notificationItems_.end(), items.begin(), items.end());
        }

        // highest progress LSN may be a tombstone and won't show up in enumeration
        auto error = storeUPtr_->GetLastCommittedSequenceNumber(copyNotificationLsn_);
        VERIFY_IS_TRUE(error.IsSuccess(), L"GetLastCommittedSequenceNumber");

        if (copyNotificationDelay_ > TimeSpan::Zero)
        {
            // Should not get here if the CopyNotificationAbortTest passes
            //
            VERIFY_IS_TRUE(false, L"Blocking OnCopyComplete()");
        }

        copyNotificationProcessed_.Set();

        return ErrorCode();
    }

    void ReplicaBase::SetCopyNotificationComplete()
    {
        copyNotificationProcessed_.Set();
    }

    void ReplicaBase::SetCopyNotificationDelay(TimeSpan const delay)
    {
        copyNotificationDelay_ = delay;
    }

    void ReplicaBase::ResetReplicationNotificationComplete()
    {
        replNotificationProcessed_.Reset();
    }

    ErrorCode ReplicaBase::OnReplicationOperation(Api::IStoreNotificationEnumeratorPtr const & itemEnumerator)
    {
        ErrorCode error(ErrorCodeValue::Success);

        ::FABRIC_SEQUENCE_NUMBER highestReplLsn = 0;

        while ((error = itemEnumerator->MoveNext()).IsSuccess())
        {
            Api::IStoreItemPtr const & itemPtr = itemEnumerator->GetCurrentItem();

            notificationItems_.erase(std::remove_if(notificationItems_.begin(), notificationItems_.end(),
                [&](DataItem const & it) -> bool { return (itemPtr->GetType() == it.Type && itemPtr->GetKey() == it.Key); }),
                notificationItems_.end());

            if (!itemPtr->IsDelete())
            {
                notificationItems_.push_back(DataItem(
                    itemPtr->GetType(),
                    itemPtr->GetKey(),
                    itemPtr->GetOperationLSN(),
                    itemPtr->GetValue(),
                    itemPtr->GetLastModifiedOnPrimaryUtc().AsFileTime));
            }

            Trace.WriteInfo(
                ReplicatedStoreTest::TestSource, 
                "OnReplicationOperation(): type = '{0}' key = '{1}' value = {2}/{3} bytes lsn = {4} delete = {5}, LastModifiedPrimary = {6}",
                itemPtr->GetType(),
                itemPtr->GetKey(),
                itemPtr->GetValueSize(),
                itemPtr->GetValue().size(),
                itemPtr->GetOperationLSN(),
                itemPtr->IsDelete(),
                itemPtr->GetLastModifiedOnPrimaryUtc().AsLargeInteger.QuadPart);

            highestReplLsn = itemPtr->GetOperationLSN();
        }

        if (error.IsError(ErrorCodeValue::EnumerationCompleted))
        {
            error = ErrorCodeValue::Success;
        }

        VERIFY(error.IsSuccess(), "OnReplicationOperation: {0}", error);

        if (highestReplLsn == replicatorCPtr_->LastNotificationLSN)
        {
            replNotificationProcessed_.Set();
        }

        return error;
    }

    void ReplicaBase::CreateDatabaseDirectoryName(__out std::wstring & dbDirectory)
    {
        wstring partitionString;
        StringWriter partitionWriter(partitionString);
        partitionWriter.Write("P_{0}", Store.PartitionId);

        wstring replicaString;
        StringWriter replicaWriter(replicaString);
        replicaWriter.Write("R_{0}", Store.ReplicaId);

        dbDirectory = Path::Combine(
            databaseDirectory_,
            Path::Combine(partitionString, replicaString));
    }

    // *******************
    // ReplicatedStoreTest
    // *******************
    bool ReplicatedStoreTest::MethodSetup()
    {
        MethodCleanup();

        Directory::Create(DatabasePrimaryDirectory);
        Directory::Create(DatabaseSecondaryDirectory);

        {
            AcquireWriteLock lock(KnownDataTypesLock);

            KnownDataTypes.clear();
        }

        StoreConfig::GetConfig().TargetCopyOperationSize = 4 * 1024;
        StoreConfig::GetConfig().AssertOnOpenSharingViolation = false;
        StoreConfig::GetConfig().OpenDatabaseRetryCount = 0;
        StoreConfig::GetConfig().EnableEndOfStreamAckOverride = false;

        return true;
    }

    bool ReplicatedStoreTest::MethodCleanup()
    {
        StoreConfig::GetConfig().Test_Reset();

        DeleteDirectory(DatabasePrimaryDirectory);
        DeleteDirectory(DatabaseSecondaryDirectory);

        return true;
    }

    void ReplicatedStoreTest::TombstoneParsingHelper(
        __in ReplicatedStore & store,
        wstring const & type, 
        wstring const & key)
    {
        Trace.WriteInfo(ReplicatedStoreTest::TestSource, "TombstoneParsingHelper({0}, {1})", type, key);

        wstring parsedType;
        wstring parsedKey;
        wstring tombstone = store.CreateTombstoneKey1(type, key);
        bool success = store.TryParseTombstoneKey1(tombstone, parsedType, parsedKey);

        VERIFY_IS_TRUE(success);
        VERIFY(type == parsedType, "type = {0} parsed = {1}", type, parsedType);
        VERIFY(key == parsedKey, "key = {0} parsed = {1}", key, parsedKey);
    }

    void ReplicatedStoreTest::DeleteDirectory(wstring const & directory)
    {
        if (Directory::Exists(directory))
        {
            int retries = 3;
            bool success = false;

            while (retries-- > 0 && !success)
            {
                auto error = Directory::Delete(directory, true);
                if(error.IsSuccess())
                {
                    success = true;
                }
                else
                {
                    Trace.WriteWarning(ReplicatedStoreTest::TestSource, "Couldn't delete directory '{0}' due to {1}", directory, error);
                    Sleep(500);
                }
            }
        }
    }
}

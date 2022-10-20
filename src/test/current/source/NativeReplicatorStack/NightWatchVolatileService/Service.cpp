// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace ktl;
using namespace Common;
using namespace V1ReplPerf;
using namespace NightWatchTXRService;

StringLiteral const TraceSource("V1RPS_Replica");
TimeSpan const HttpServerOpenCloseTimeout = TimeSpan::FromMinutes(2);
ULONG const numberOfParallelHttpRequests = 100;

#define WRITEINFO(message) Trace.WriteInfo(TraceSource, "{0} at Partition = {1} Replica = {2}", message, partitionId_, replicaId_)

V1ReplPerf::Service::Service(
    FABRIC_PARTITION_ID partitionId,
    FABRIC_REPLICA_ID replicaId,
    Common::ComponentRootSPtr const & root)
    : instanceId_(SequenceNumber::GetNext())
    , root_(root)
    , replicaId_(replicaId)
    , partitionId_(Guid(partitionId).ToString())
#if defined(PLATFORM_UNIX)
    , httpListenAddress_(wformatString("http://0.0.0.0:{0}/{1}/{2}/{3}", TXRStatefulServiceBase::Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
#else
    , httpListenAddress_(wformatString("http://+:{0}/{1}/{2}/{3}", TXRStatefulServiceBase::Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
#endif
    , changeRoleEndpoint_(wformatString("http://{0}:{1}/{2}/{3}/{4}", TXRStatefulServiceBase::Helpers::GetFabricNodeIpAddressOrFqdn(), TXRStatefulServiceBase::Helpers::GetHttpPortFromConfigurationPackage(), Guid(partitionId), replicaId, instanceId_))
    , replicaRole_(FABRIC_REPLICA_ROLE_UNKNOWN)
    , isOpen_(false)
    , httpServerSPtr_()
    , partition_()
    , replicator_()
    , stateLock_()
    , stateProviderCPtr_()
    , pumpState_(PumpNotStarted)
    , pumpingCompleteEvent_(true)
    , testInProgress_(false)
    , testResultSptr_(nullptr)
{
    Trace.WriteInfo(
        TraceSource,
        "NightWatchVolatileService.Ctor. HttpListenAddress: {0}. ServiceEndpoint: {1}",
        httpListenAddress_,
        changeRoleEndpoint_);
}

V1ReplPerf::Service::~Service()
{
}

HttpServer::IHttpServerSPtr CreateAndOpenHttpServer(
    __in ComponentRootSPtr const & root,
    __in wstring const & listenAddress,
    __in ULONG numberOfParallelRequests)
{
    AutoResetEvent openEvent;

    shared_ptr<HttpServer::HttpServerImpl> httpServer = make_shared<HttpServer::HttpServerImpl>(
        *root,
        listenAddress,
        numberOfParallelRequests,
        Transport::SecurityProvider::None);

    httpServer->BeginOpen(
        HttpServerOpenCloseTimeout,
        [&openEvent, &httpServer](AsyncOperationSPtr const &operation)
    {
        ErrorCode e = httpServer->EndOpen(operation);
        if (e.IsSuccess())
        {
            Trace.WriteInfo(
                TraceSource,
                "HttpServer Open successful");
        }
        else
        {
            Trace.WriteError(
                TraceSource,
                "HttpServer Open failed - {0}",
                e);
        }

        openEvent.Set();
    });

    openEvent.WaitOne();

    return httpServer;
}

void CloseHttpServer(__in HttpServer::IHttpServerSPtr & httpServer)
{
    if (!httpServer)
    {
        return;
    }

    HttpServer::HttpServerImpl & httpServerImpl = (HttpServer::HttpServerImpl&)(*httpServer);
    AutoResetEvent closeEvent;

    httpServerImpl.BeginClose(
        HttpServerOpenCloseTimeout,
        [&closeEvent, &httpServerImpl, &httpServer](AsyncOperationSPtr const &operation)
    {
        ErrorCode e = httpServerImpl.EndClose(operation);

        if (e.IsSuccess())
        {
            Trace.WriteInfo(
                TraceSource,
                "HttpServer close successful");

            httpServer.reset();
        }
        else
        {
            Trace.WriteError(
                TraceSource,
                "HttpServer Close failed - {0}",
                e);
        }

        closeEvent.Set();
    },
        nullptr);

    closeEvent.WaitOne();
}

Common::ErrorCode V1ReplPerf::Service::OnHttpPostRequest(Common::ByteBufferUPtr && body, Common::ByteBufferUPtr & responseBody)
{
    Trace.WriteInfo(
        TraceSource,
        "Received http POST with body length {0}",
        body->size());

    NightWatchTestParameters testParameters;
    JsonSerializerFlags flags = JsonSerializerFlags::EnumInStringFormat; // This will have the enum in string format

    auto error = JsonHelper::Deserialize<NightWatchTestParameters>(testParameters, body, flags);

    testParameters.PopulateUpdateOperationValue();

    Trace.WriteInfo(
        TraceSource,
        "Deserialized http POST request with object {0} with Error Code {1}",
        testParameters,
        error);

    if (!error.IsSuccess())
    {
        auto response = TestError(L"Serialization error.", TestStatus::Enum::Invalid);
        return JsonHelper::Serialize<TestError>(response, responseBody, flags);
    }

    if (testParameters.Action != TestAction::Enum::Run && testParameters.Action != TestAction::Enum::Status)
    {
        auto response = TestError(L"Wrong action.", TestStatus::Enum::Invalid);
        return JsonHelper::Serialize<TestError>(response, responseBody, flags);
    }

    if (testParameters.Action == TestAction::Enum::Run)
    {
        if (testInProgress_)
        {
            auto response = TestError(L"Test is already running. You cannot start a new one right now.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
        else
        {
            StartTest(testParameters);
            auto response = TestError(L"Test is scheduled.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
    }
    else if (testParameters.Action == TestAction::Enum::Status)
    {
        if (testInProgress_)
        {
            auto response = TestError(L"Test is still running. Check back later.", TestStatus::Enum::Running);
            return JsonHelper::Serialize<TestError>(response, responseBody, flags);
        }
        else if (!testInProgress_)
        {
            if (testResultSptr_ == nullptr)
            {
                auto response = TestError(L"No test is scheduled to run.", TestStatus::Enum::NotStarted);
                return JsonHelper::Serialize<TestError>(response, responseBody, flags);
            }
            else if (testResultSptr_->Exception != nullptr)
            {
                auto response = TestError(L"Exception has occured during the run.", TestStatus::Enum::Finished);
                return JsonHelper::Serialize<TestError>(response, responseBody, flags);
            }
            else
            {
                Trace.WriteInfo(
                    TraceSource,
                    "Test result is ready: {0}",
                    testResultSptr_->ToString());
                return JsonHelper::Serialize<PerfResult>(*testResultSptr_, responseBody, flags);
            }
        }
    }

    return error;
}

void V1ReplPerf::Service::StartTest(__in NightWatchTestParameters const & testParameters)
{
    KAllocator & allocator = ktlSystem_->PagedAllocator();

    testInProgress_ = true;

    //resetting previous run results
    testResultSptr_ = nullptr;

    try
    {
        auto root = this->CreateComponentRoot();
        ComPointer<IFabricStateReplicator> replicator;
        {
            AcquireReadLock grab(stateLock_);
            replicator = replicator_;
        }

        Threadpool::Post(
            [root, this, replicator, testParameters]
        {
            if (this->replicaRole_ != ::FABRIC_REPLICA_ROLE_PRIMARY ||
                this->isOpen_ == false)
                return;

            WRITEINFO("Starting test execution");
            testResultSptr_ = Test::Execute(testParameters, replicator, this->ktlSystem_->PagedAllocator());
            testInProgress_ = false;
        },
            TimeSpan::FromSeconds(60));
    }
    catch (...)
    {
        testResultSptr_ = PerfResult::Create(
            Data::Utilities::SharedException::Create(allocator).RawPtr(),
            allocator);
        testInProgress_ = false;
    }
}


void V1ReplPerf::Service::OnOpen(IFabricStatefulServicePartition *statefulServicePartition, ComPointer<IFabricStateReplicator> const& replicator)
{
    WRITEINFO("OnOpen called");

    ComPointer<IFabricInternalStatefulServicePartition> internalPartition;
    statefulServicePartition->QueryInterface(IID_IFabricInternalStatefulServicePartition, internalPartition.VoidInitializationAddress());

    ComPointer<IFabricStatefulServicePartition3> partitionV3;
    partitionV3.SetAndAddRef(static_cast<IFabricStatefulServicePartition3*>(statefulServicePartition));

    HANDLE ktlSystem = nullptr;
    internalPartition->GetKtlSystem(&ktlSystem);
    ktlSystem_ = (KtlSystem*)ktlSystem;

    {
        AcquireWriteLock grab(stateLock_);
        isOpen_ = true;
        partition_ = partitionV3;
        replicator_ = replicator;
    }

    httpServerSPtr_ = CreateAndOpenHttpServer(
        root_,
        httpListenAddress_,
        numberOfParallelHttpRequests);

    HttpRequestProcessCallback postRequestCallback =
        [this](Common::ByteBufferUPtr && body, Common::ByteBufferUPtr & responseBody)
    {
        return OnHttpPostRequest(move(body), responseBody);
    };

    TestClientRequestHandlerSPtr handlerSPtr = TestClientRequestHandler::Create(
        postRequestCallback);

    ErrorCode errorCode = httpServerSPtr_->RegisterHandler(L"/", handlerSPtr);

    if (!errorCode.IsSuccess())
    {
        Trace.WriteError(
            TraceSource,
            "Failed to register request handler with error {0}",
            errorCode);

        //hr = errorCode.ToHResult();
    }
}

std::wstring V1ReplPerf::Service::OnChangeRole(FABRIC_REPLICA_ROLE newRole)
{
    Trace.WriteInfo(
        TraceSource, 
        "Change role called for Service partitionid = {0} replica = {1} OldRole = {2} NewRole = {3}", 
        partitionId_, 
        replicaId_,
        replicaRole_,
        newRole);

    bool isGetOperationsPumpActive = (replicaRole_ == ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY || replicaRole_ == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY);
    replicaRole_ = newRole;

    auto root = this->CreateComponentRoot();
    ComPointer<IFabricStateReplicator> replicator;
    {
        AcquireReadLock grab(stateLock_);
        replicator = replicator_;
    }

    if (!isGetOperationsPumpActive)
    {
        if (replicaRole_ == ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
        {
            Trace.WriteInfo(TraceSource, "Starting copy operation pump.");
            Threadpool::Post([root, this, replicator]
            {
                StartCopyOperationPump(replicator);
            });
        }
        else if (replicaRole_ == ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
        {
            Trace.WriteInfo(TraceSource, "Starting replication operation pump.");
            Threadpool::Post([root, this, replicator]
            {
                StartReplicationOperationPump(replicator);
            });
        }
    }

    if (replicaRole_ == ::FABRIC_REPLICA_ROLE_PRIMARY)
    {
        pumpingCompleteEvent_.WaitOne();
        Trace.WriteInfo(TraceSource, "Primary change role.");
        /*Threadpool::Post(
            [root, this, replicator]
            {
                if (this->replicaRole_ != ::FABRIC_REPLICA_ROLE_PRIMARY || 
                    this->isOpen_ == false)
                    return;

                Test::Execute(replicator);
            },
            TimeSpan::FromSeconds(60));*/
    }

    Trace.WriteInfo(
        TraceSource,
        "End of OnChangeRole newRole = {0}",
        newRole
    );

    return changeRoleEndpoint_;
}

void V1ReplPerf::Service::StartCopyOperationPump(ComPointer<IFabricStateReplicator> const & replicator)
{
    ComPointer<IFabricOperationStream> stream;

    HRESULT hr = replicator->GetCopyStream(stream.InitializationAddress());
    if (FAILED(hr)) 
    {
        if (hr == ErrorCodeValue::InvalidState || hr == ErrorCodeValue::ObjectClosed)
        {
            // The replicator state changed due to close/ChangeRole
            Trace.WriteWarning(TraceSource, "GetCopyStream: Error {0}", hr);
        }
        else
        {
            Common::Assert::CodingError("GetCopyStream failed with HR {0}", hr);
        }
    }
    else
    {
        pumpState_ = PumpCopy;
        BeginGetOperations(replicator, stream);
    }
}

void V1ReplPerf::Service::StartReplicationOperationPump(ComPointer<IFabricStateReplicator> const & replicator)
{
    ComPointer<IFabricOperationStream> stream;

    HRESULT hr = replicator->GetReplicationStream(stream.InitializationAddress());
    if (FAILED(hr)) 
    {
        if (hr == ErrorCodeValue::InvalidState || hr == ErrorCodeValue::ObjectClosed)
        {
            // The replicator state changed due to close/ChangeRole
            Trace.WriteWarning(TraceSource, "GetReplicationStream: Error {0}", hr);
        }
        else
        {
            Common::Assert::CodingError("GetReplicationStream failed with HR {0}", hr);
        }
    }
    else
    {
        pumpState_ = PumpReplication;
        BeginGetOperations(replicator, stream);
    }
}

void V1ReplPerf::Service::BeginGetOperations(ComPointer<IFabricStateReplicator> const & replicator, ComPointer<IFabricOperationStream> const & stream)
{
    BOOLEAN completedSynchronously;
    pumpingCompleteEvent_.Reset();

    do
    {
        auto root = CreateComponentRoot();
        ComPointer<ComAsyncOperationCallbackHelper> callback = make_com<ComAsyncOperationCallbackHelper>(
            [this, stream, replicator, root](IFabricAsyncOperationContext * context)
        {
            BOOLEAN inCallbackCompletedSynchronously = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(context);
            if(!inCallbackCompletedSynchronously)
            {
                if (FinishProcessingOperation(context, replicator, stream))
                {
                    BeginGetOperations(replicator, stream);
                }
            }
        });

        ComPointer<IFabricAsyncOperationContext> beginContext;
        HRESULT hr = stream->BeginGetOperation(callback.GetRawPointer(), beginContext.InitializationAddress());

        if (FAILED(hr)) { Common::Assert::CodingError("BeginGetOperation failed with HR {0}", hr); }

        completedSynchronously = ComAsyncOperationCallbackHelper::GetCompletedSynchronously(beginContext.GetRawPointer());

        if(completedSynchronously)
        {
            if (!FinishProcessingOperation(beginContext.GetRawPointer(), replicator, stream))
            {
                break;
            }
        }
    }
    while(completedSynchronously);
}

bool V1ReplPerf::Service::FinishProcessingOperation(IFabricAsyncOperationContext * context, ComPointer<IFabricStateReplicator> const & replicator, ComPointer<IFabricOperationStream> const & stream)
{
    ComPointer<IFabricOperation> operation;
    HRESULT hr = stream->EndGetOperation(context, operation.InitializationAddress());

    if (FAILED(hr)) { Common::Assert::CodingError("EndGetOperation failed with HR {0}", hr); }

    if (operation)
    {
        ULONG bufferCount = 0;
        FABRIC_OPERATION_DATA_BUFFER const * replicaBuffers = nullptr;

        hr = operation->GetData(&bufferCount, &replicaBuffers);

        if (FAILED(hr)) { Common::Assert::CodingError("GetData failed with HR {0}", hr); }

        ASSERT_IF(bufferCount > 1, "Unexpected multiple buffers");

        operation->Acknowledge();
    }
    else
    {
        // Pumping complete set the reset event.
        pumpingCompleteEvent_.Set();

        if (pumpState_ == PumpCopy)
        {
            // Start replication pump
            // Post this since StartReplicationOperationPump can get blocked due to a signal
            auto root = this->CreateComponentRoot();
            Threadpool::Post([this, replicator, root]
            {
                StartReplicationOperationPump(replicator);    
            });
        }

        return false;
    }

    return true;
}

void V1ReplPerf::Service::OnClose()
{
    WRITEINFO("OnClose called");
    Cleanup();
}

void V1ReplPerf::Service::OnAbort()
{
    WRITEINFO("OnAbort called");
    Cleanup();
}

void V1ReplPerf::Service::Cleanup()
{
    AcquireWriteLock grab(stateLock_);

    //CloseHttpServer(httpServerSPtr_);

    if (!isOpen_)
        return;

    isOpen_ = false;
    replicator_.Release();
    partition_.Release();
}

static const GUID CLSID_TestGetNextOperation = { 0x86aef2e3, 0xf375, 0x4440, { 0xab, 0x1d, 0xa1, 0xbe, 0x58, 0x8d, 0x92, 0x18 } };

class V1ReplPerf::Service::ComGetNextOperation : public ComAsyncOperationContext
{
    DENY_COPY(ComGetNextOperation)

        COM_INTERFACE_LIST2(
        ComTestGetNextOperation,
        IID_IFabricAsyncOperationContext,
        IFabricAsyncOperationContext,
        CLSID_TestGetNextOperation,
        ComGetNextOperation)

public:
    explicit ComGetNextOperation(){ }

    virtual ~ComGetNextOperation(){ }

    HRESULT Initialize(
        __in IFabricAsyncOperationCallback * inCallback)
    {
        //Dont need to pass in root pointer. The callback posted to threadpool will keep it alive. 
        HRESULT hr = this->ComAsyncOperationContext::Initialize(ComponentRootSPtr(), inCallback);
        if (FAILED(hr)) { return hr; }
        return S_OK;
    }

    static HRESULT End(
        __in IFabricAsyncOperationContext * context,
        __out IFabricOperationData ** operation)
    {
        if (context == NULL) { return E_POINTER; }

        ComPointer<ComGetNextOperation> thisOperation(context, CLSID_TestGetNextOperation);
        HRESULT hr = thisOperation->ComAsyncOperationContext::End();;
        if (FAILED(hr)) { return hr; }
        *operation = thisOperation->innerComOperationPointer_.DetachNoRelease();
        return thisOperation->Result;
    }

    void SetOperation(ComPointer<OperationData> && copy)
    {
        innerComOperationPointer_.SetNoAddRef(copy.DetachNoRelease());
    }

    bool IsInnerComOperationPointerNull()
    {
        return innerComOperationPointer_.GetRawPointer() == nullptr;
    }

private:
    virtual void OnStart(AsyncOperationSPtr const &)
    {
    }

    ComPointer<OperationData> innerComOperationPointer_;
};

class V1ReplPerf::Service::ComAsyncEnumOperationData : public IFabricOperationDataStream, ComUnknownBase
{
    DENY_COPY(ComAsyncEnumOperationData)
        COM_INTERFACE_LIST1(
        ComTestAsyncEnumOperationData,
        IID_IFabricOperationDataStream,
        IFabricOperationDataStream)

public:

    ComAsyncEnumOperationData(Service & storeService, vector<__int64> && keys, wstring partitionId, FABRIC_SEQUENCE_NUMBER uptoSequenceNumber) 
        : storeService_(storeService), root_(storeService_.CreateComponentRoot()), keys_(keys), index_(0), partitionId_(partitionId), uptoSequenceNumber_(uptoSequenceNumber)
    {
    }

    virtual ~ComAsyncEnumOperationData() 
    {
    }

    HRESULT STDMETHODCALLTYPE BeginGetNext(
        /*[in]*/ IFabricAsyncOperationCallback * callback,
        /*[out, retval]*/ IFabricAsyncOperationContext ** context)
    {
            ComPointer<ComGetNextOperation> operation = make_com<ComGetNextOperation>();
            HRESULT hr = operation->Initialize(callback);
            if (FAILED(hr)) { return hr; }

            ComPointer<ComGetNextOperation> operationCopy(operation);
            MoveCPtr<ComGetNextOperation> mover(move(operationCopy));

            hr = ComAsyncOperationContext::StartAndDetach(move(operation), context);
            if (FAILED(hr)) { return hr; }

            GetNextOperation(move(mover.TakeSPtr())); 

            return S_OK;
    }

    HRESULT STDMETHODCALLTYPE EndGetNext(
        /*[in]*/ IFabricAsyncOperationContext * context,
        /*[out, retval]*/ IFabricOperationData ** operation)
    {
        auto castedContext = dynamic_cast<ComCompletedAsyncOperationContext*>(context);
        if(castedContext != nullptr)
        {
            return ComCompletedAsyncOperationContext::End(context);
        }
        else
        {
            return ComGetNextOperation::End(context, operation);
        }
    }

private:
    Service & storeService_;
    ComponentRootSPtr root_;
    vector<__int64> keys_;
    size_t index_;
    wstring partitionId_;
    FABRIC_SEQUENCE_NUMBER uptoSequenceNumber_;

    void GetNextOperation(ComPointer<ComGetNextOperation> && operation)
    {
        if(index_ == keys_.size())
        {
            ASSERT_IFNOT(operation->IsInnerComOperationPointerNull(), "Operation should be NULL after copy is done");
        }

        ComAsyncOperationContextCPtr baseOperation;
        baseOperation.SetNoAddRef(operation.DetachNoRelease());
        if (baseOperation->TryStartComplete())
        {
            baseOperation->FinishComplete(baseOperation);
        }
    }
};

HRESULT STDMETHODCALLTYPE V1ReplPerf::Service::UpdateEpoch(
    /* [in] */ FABRIC_EPOCH const * epoch,
    /* [in] */ FABRIC_SEQUENCE_NUMBER previousEpochLastSequenceNumber)
{
    UNREFERENCED_PARAMETER(previousEpochLastSequenceNumber);
    UNREFERENCED_PARAMETER(epoch);
    return S_OK;
}


HRESULT STDMETHODCALLTYPE V1ReplPerf::Service::GetCurrentProgress(
    /* [retval][out] */ FABRIC_SEQUENCE_NUMBER *sequenceNumber)
{
    //Since there is no persisted data progreess is always 0
    *sequenceNumber = 0;
    return S_OK;
}


//The test does not recover from data loss
HRESULT V1ReplPerf::Service::BeginOnDataLoss(
    /* [in] */ IFabricAsyncOperationCallback *callback,
    /* [retval][out] */ IFabricAsyncOperationContext **context)
{
    ComPointer<ComCompletedAsyncOperationContext> operation = make_com<ComCompletedAsyncOperationContext>();

    HRESULT hr = operation->Initialize(
        ComponentRootSPtr(),
        callback);
    if (FAILED(hr)) { return hr; }

    ComAsyncOperationContextCPtr baseOperation;
    baseOperation.SetNoAddRef(operation.DetachNoRelease());
    hr = baseOperation->Start(baseOperation);
    if (FAILED(hr)) { return hr; }

    *context = baseOperation.DetachNoRelease();
    return S_OK;
}

HRESULT V1ReplPerf::Service::EndOnDataLoss(
    /* [in] */ IFabricAsyncOperationContext *context,
    /* [retval][string][out] */ BOOLEAN * isStateChanged)
{
    *isStateChanged = FALSE;
    return ComCompletedAsyncOperationContext::End(context);
}

HRESULT V1ReplPerf::Service::GetCopyContext(
    /*[out, retval]*/ IFabricOperationDataStream ** copyContextEnumerator)
{
    vector<__int64> keys;
    *copyContextEnumerator = make_com<ComAsyncEnumOperationData>(*this, move(keys), partitionId_, 1).DetachNoRelease();
    return S_OK;
}

//TODO: think about batching the copy operations
HRESULT V1ReplPerf::Service::GetCopyState(
    /*[in]*/ FABRIC_SEQUENCE_NUMBER uptoSequenceNumber,
    /*[in]*/ IFabricOperationDataStream *,
    /*[out, retval]*/ IFabricOperationDataStream ** copyStateEnumerator)
{
    vector<__int64> keys;
    *copyStateEnumerator = make_com<ComAsyncEnumOperationData>(*this, move(keys), partitionId_, uptoSequenceNumber).DetachNoRelease();
    return S_OK;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace ServiceModel;
using namespace Transport;
using namespace Hosting2;
using namespace Query;
using namespace Store;
using namespace SystemServices;
using namespace Management;
using namespace Management::HealthManager;
using namespace ServiceModel;
using namespace std;
using namespace Testability;
using namespace ClientServerTransport;

StringLiteral const TraceType("TestabilitySubsystemSubsystem");

class TestabilitySubsystem::QueryMessageHandlerJobItem : 
    public CommonTimedJobItem<TestabilitySubsystem>
{
public:
    QueryMessageHandlerJobItem(
        TimeSpan timeout,
        std::function<void(TestabilitySubsystem &)> const processCallback,
        std::function<void(TestabilitySubsystem &)> const timeOutCallback)
        : CommonTimedJobItem(timeout)
        , processCallback_(processCallback)
        , timeOutCallback_(timeOutCallback)
    {
    }

    virtual void Process(TestabilitySubsystem & testabilitySubsystem) override
    {
        processCallback_(testabilitySubsystem);
    }

    virtual void OnTimeout(TestabilitySubsystem &testabilitySubsystem) override
    {
        timeOutCallback_(testabilitySubsystem);
    }

private:
    std::function<void(TestabilitySubsystem &)> const processCallback_;
    std::function<void(TestabilitySubsystem &)> const timeOutCallback_;
};

class TestabilitySubsystem::TestabilitySubsystemRequestHandler : 
    public TimedAsyncOperation
{
public:
    TestabilitySubsystemRequestHandler(
        __in TestabilitySubsystem& owner,
        QueryNames::Enum queryName,
        QueryArgumentMap const & queryArgs,
        ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , queryName_(queryName)
        , queryArgs_(queryArgs)
        , activityId_(activityId)
        , owner_(owner)
        , reply_()
    {
    }

    virtual ~TestabilitySubsystemRequestHandler() = default;

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation,
        __out Transport::MessageUPtr &reply)
    {
        auto casted = AsyncOperation::End<TestabilitySubsystemRequestHandler>(asyncOperation);
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }
        return casted->Error;
    }

protected:
    
    void OnStart(AsyncOperationSPtr const& thisSPtr)
    {
        auto jobItem = make_unique<QueryMessageHandlerJobItem>(
            this->get_RemainingTime(),
            [thisSPtr, this](TestabilitySubsystem &) { this->ProcessQuery(thisSPtr); },
            [thisSPtr, this](TestabilitySubsystem &) { this->OnTimeOut(thisSPtr); });
        
        bool success = owner_.queryJobQueue_->Enqueue(move(jobItem));

        if (!success)
        {
            WriteWarning(
                TraceType,
                owner_.Root.TraceId,
                "Failed to enqueue QueryRequest to JobQueue. ActivityId: {0}", activityId_);
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }
    }

    void ProcessQuery(AsyncOperationSPtr const& thisSPtr)
    {
        ErrorCode error = ErrorCodeValue::Success;
        QueryResult result;
        bool completeAsyncOperation = true;

        switch (queryName_)
        {
        case QueryNames::StopNode:
            error = owner_.StopNode(queryArgs_, activityId_, result);
            break;
        case QueryNames::AddUnreliableTransportBehavior:
            error = owner_.AddUnreliableTransportBehavior(queryArgs_, activityId_, result);
            break;
        case QueryNames::RemoveUnreliableTransportBehavior:
            error = owner_.RemoveUnreliableTransportBehavior(queryArgs_, activityId_, result);
            break;
        case QueryNames::GetTransportBehaviors:
            error = owner_.GetTransportBehaviors(queryArgs_, activityId_, result);
            break;
        case QueryNames::AddUnreliableLeaseBehavior:
            error = owner_.AddUnreliableLeaseBehavior(queryArgs_, activityId_, result);
            break;
        case QueryNames::RemoveUnreliableLeaseBehavior:
            error = owner_.RemoveUnreliableLeaseBehavior(queryArgs_, activityId_, result);
            break;
        case QueryNames::RestartDeployedCodePackage:
            error = RestartDeployedCodePackage(thisSPtr);
            completeAsyncOperation = error.IsSuccess() ? false : true;
            break;
        default:
            error = ErrorCodeValue::InvalidArgument;
            break;
        }

        if (completeAsyncOperation)
        {
            reply_ = make_unique<Message>(result);
            TryComplete(thisSPtr, error);
        }
    }

    void OnTimeOut(AsyncOperationSPtr const& thisSPtr)
    {
        WriteWarning(
            TraceType,
            owner_.Root.TraceId,
            "Failed to enqueue QueryRequest to JobQueue. ActivityId: {0}", activityId_);
        TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
        return;
    }

private:

    ErrorCode RestartDeployedCodePackage(
        AsyncOperationSPtr const& thisSPtr)
    {
        WriteInfo(
            TraceType,
            owner_.Root.TraceId,
            "{0}: TestabilitySubsystem::RestartDeployedCodePackage",
            activityId_);
        wstring instanceId;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::CodePackage::InstanceId, instanceId) ||
            instanceId.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        int64 codePackageInstanceId;
        if (!StringUtility::TryFromWString<int64>(instanceId, codePackageInstanceId))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring applicationNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::Application::ApplicationName, applicationNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring serviceManifestNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ServiceType::ServiceManifestName, serviceManifestNameArgument))
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring codePackageNameArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::CodePackage::CodePackageName, codePackageNameArgument) ||
            codePackageNameArgument.empty())
        {
            return ErrorCodeValue::InvalidArgument;
        }

        wstring servicePackageActivationIdArgument;
        if (!queryArgs_.TryGetValue(QueryResourceProperties::ServicePackage::ServicePackageActivationId, servicePackageActivationIdArgument))
        {
            WriteNoise(
                TraceType,
                owner_.Root.TraceId,
                "{0}: RestartDeployedCodePackage - ServicePackageActivationId not provided. Using default.",
                activityId_);
        }

        Hosting2::IHostingSubsystemSPtr hostingSubsystem = owner_.hostingSubSytem_.lock();
        if (hostingSubsystem == nullptr)
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "{0}: Error - hostingSubsystem on TestabilitySubsystem is invalid",
                activityId_);
            return ErrorCodeValue::InvalidAddress;
        }

        auto asyncCallback = [=](AsyncOperationSPtr const& operation){
            OnTestabilityRestartDeployedCodePackageComplete(operation, false);
        };

        auto operation = hostingSubsystem->BeginRestartDeployedPackage(
            codePackageInstanceId,
            applicationNameArgument,
            serviceManifestNameArgument,
            servicePackageActivationIdArgument,
            codePackageNameArgument,
            activityId_,
            this->get_RemainingTime(),
            asyncCallback,
            thisSPtr);
        OnTestabilityRestartDeployedCodePackageComplete(operation, true);
    
        return ErrorCodeValue::Success;
    }

    void OnTestabilityRestartDeployedCodePackageComplete(AsyncOperationSPtr const& operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
        ErrorCode error = ErrorCodeValue::Success;
        Hosting2::IHostingSubsystemSPtr hostingSubsystem = owner_.hostingSubSytem_.lock();
        if (hostingSubsystem != nullptr)
        {
            hostingSubsystem->EndRestartDeployedPackage(operation, reply_);
        }
        else
        {
            WriteInfo(
                TraceType,
                owner_.Root.TraceId,
                "{0}: Error - hostingSubsystem on TestabilitySubsystem is invalid",
                activityId_);
            error = ErrorCodeValue::InvalidAddress;
        }
        TryComplete(operation->Parent, error);
    }

protected:
    QueryArgumentMap queryArgs_;
    QueryNames::Enum queryName_;
    ActivityId const activityId_;
    TestabilitySubsystem & owner_;
    Transport::MessageUPtr reply_;
};

class TestabilitySubsystem::TestabilitySubsystemQueryMessageHandler :
    public TimedAsyncOperation
{
public:

    TestabilitySubsystemQueryMessageHandler(
        __in TestabilitySubsystem & owner,
        __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
        MessageUPtr && request,
        RequestReceiverContextUPtr && requestContext,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root) : 
        TimedAsyncOperation(timeout, callback, root)
        , owner_(owner)
        , queryMessageHandler_(queryMessageHandler)
        , error_(ErrorCodeValue::Success)
        , request_(move(request))
        , requestContext_(move(requestContext))
    {
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);
        auto operation = this->BeginAcceptRequest(
            this->RemainingTime,
            [this](AsyncOperationSPtr const & operation) { this->OnAcceptRequestComplete(operation, false); },
            thisSPtr);
        this->OnAcceptRequestComplete(operation, true);
    }

    void OnAcceptRequestComplete(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        AsyncOperationSPtr const & thisSPtr = operation->Parent;

        auto error = this->EndAcceptRequest(operation);
        if (!error.IsSuccess())
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    static ErrorCode End(
        AsyncOperationSPtr const & asyncOperation,
        __out MessageUPtr & reply,
        __out RequestReceiverContextUPtr & requestContext)
    {
        auto casted = AsyncOperation::End<TestabilitySubsystemQueryMessageHandler>(asyncOperation);

        if (casted->Error.IsSuccess())
        {
            if (casted->reply_)
            {
                reply = move(casted->reply_);
            }
        }
        // request context is used to reply with errors to client, so always return it
        requestContext = move(casted->requestContext_);
        return casted->Error;
    }

protected:

    Common::AsyncOperationSPtr BeginAcceptRequest(
        Common::TimeSpan const timespan,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return queryMessageHandler_->BeginProcessQueryMessage(*request_, timespan, callback, parent);
    }

    Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &operation)
    {
        MessageUPtr reply;
        ErrorCode error = queryMessageHandler_->EndProcessQueryMessage(operation, reply);
        if (error.IsSuccess())
        {
            reply_ = std::move(reply);
        }

        auto thisSPtr = this->shared_from_this();
        this->TryComplete(thisSPtr, error);

        return error;
    }

private:

    Query::QueryMessageHandlerUPtr & queryMessageHandler_;
    TestabilitySubsystem & owner_;
    Common::ErrorCodeValue::Enum error_;
    Transport::MessageUPtr request_;
    Transport::MessageUPtr reply_;
    Federation::RequestReceiverContextUPtr requestContext_;
};

class TestabilitySubsystem::OpenAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::TestabilityComponent>
{
    DENY_COPY(OpenAsyncOperation)

public:
    OpenAsyncOperation(
        TestabilitySubsystem & owner,
        TimeSpan const,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner)
    {
    }

    virtual ~OpenAsyncOperation() = default;

    static ErrorCode OpenAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode err = owner_.OnOpen();
        TryComplete(thisSPtr, err);
    }

private:
    TestabilitySubsystem & owner_;
};

class TestabilitySubsystem::CloseAsyncOperation
    : public AsyncOperation,
    TextTraceComponent<TraceTaskCodes::TestabilityComponent>
{
    DENY_COPY(CloseAsyncOperation)

public:
    CloseAsyncOperation(
        TestabilitySubsystem & owner,
        TimeSpan const,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner)
    {
    }

    virtual ~CloseAsyncOperation() = default;

    static ErrorCode CloseAsyncOperation::End(
        AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<CloseAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        owner_.OnClose();
        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::Success));
    }

private:
    TestabilitySubsystem & owner_;
};

// ********************************************************************************************************************
// TestabilitySubsystem Implementation
//
// ********************************************************************************************************************
ITestabilitySubsystemSPtr Testability::TestabilitySubsystemFactory(
    TestabilitySubsystemConstructorParameters & parameters)
{
    return std::make_shared<TestabilitySubsystem>(
        *parameters.Root,
        *parameters.Federation,
        parameters.NodeConfig,
        parameters.HostingSubSytem);
}

TestabilitySubsystem::TestabilitySubsystem(
    __in ComponentRoot const & root,
    __in Federation::FederationSubsystem & federation,
    __in FabricNodeConfigSPtr nodeConfig,
    __in Hosting2::IHostingSubsystemWPtr hostingSubSytem)
    : RootedObject(root)
    , federation_(federation)
    , nodeConfig_(nodeConfig)
    , hostingSubSytem_(hostingSubSytem)
    , queryMessageHandler_(nullptr)
    , unreliableTx_(UnreliableTransportHelper::CreateSPtr(nodeConfig))
    , unreliableLease_(make_unique<UnreliableLeaseHelper>())
{
}

void TestabilitySubsystem::InitializeHealthReportingComponent(
    Client::HealthReportingComponentSPtr const & healthClient)
{
    unreliableTx_->InitializeHealthReportingComponent(healthClient);
}

AsyncOperationSPtr TestabilitySubsystem::BeginTestabilityOperation(
    QueryNames::Enum queryName,
    QueryArgumentMap const & queryArgs,
    ActivityId const & activityId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto operation = AsyncOperation::CreateAndStart<TestabilitySubsystem::TestabilitySubsystemRequestHandler>(
        *this,
        queryName,
        queryArgs,
        activityId,
        timeout,
        callback,
        parent);
    return operation;
}

ErrorCode TestabilitySubsystem::EndTestabilityOperation(
    AsyncOperationSPtr const &operation,
    __out MessageUPtr & reply)
{
    return TestabilitySubsystemRequestHandler::End(operation, reply);
}

AsyncOperationSPtr TestabilitySubsystem::OnBeginOpen(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode TestabilitySubsystem::OnEndOpen(
    AsyncOperationSPtr const & operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr TestabilitySubsystem::OnBeginClose(
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(
        *this,
        timeout,
        callback,
        parent);
}

ErrorCode TestabilitySubsystem::OnEndClose(
    AsyncOperationSPtr const & operation)
{
    return CloseAsyncOperation::End(operation);
}

ErrorCode TestabilitySubsystem::OnOpen()
{
    unreliableTx_->Open();

    // The lifetime of this replica is managed by the runtime (FabricRuntime)
    // and not the fabric root (FabricNode)
    //
    auto selfRoot = this->CreateComponentRoot();
    queryMessageHandler_ = make_unique<QueryMessageHandler>(*selfRoot, QueryAddresses::TestabilityAddressSegment);
    queryMessageHandler_->RegisterQueryHandler(
        [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, Common::ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
    {
        return this->BeginTestabilityOperation(queryName, queryArgs, activityId, timeout, callback, parent);
    },
        [this](Common::AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
    {
        return this->EndTestabilityOperation(operation, reply);
    });

    auto error = queryMessageHandler_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType,
            "{0} QueryMessageHandler failed to open with error {1}",
            this->TraceId,
            error);
    }

    queryJobQueue_ = make_unique<CommonTimedJobQueue<TestabilitySubsystem>>(
        L"TestabilitySubsystem." + this->Root.TraceId,
        *this,
        false,
        TestabilityConfig::GetConfig().MaxQueryOperationThreads,
        JobQueuePerfCounters::CreateInstance(L"TestabilitySubsystem." + this->Root.TraceId),
        TestabilityConfig::GetConfig().QueryQueueSize);

    // Register the one way and request-reply message handlers
    federation_.RegisterMessageHandler(
        Actor::TestabilitySubsystem,
        [this](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        {
            WriteError(
                TraceType,
                "{0} received a oneway message: {1}",
                Root.TraceId,
                *message);
            oneWayReceiverContext->Reject(ErrorCodeValue::InvalidMessage);
        },
        [this](Transport::MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->RequestMessageHandler(message, requestReceiverContext);
        },
        false /*dispatchOnTransportThread*/);
    
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Checking zombie mode");

    bool isInZombieMode = false;
    
    wstring workingDir = nodeConfig_->WorkingDir;
    wstring startStopFileName = nodeConfig_->StartStopFileName;
    wstring absolutePathToStartStopFile = Path::Combine(workingDir, startStopFileName);        
        
    WriteInfo(
        TraceType,
        Root.TraceId,
        "Path to file is {0}", absolutePathToStartStopFile);
    if (File::Exists(absolutePathToStartStopFile))
    {
        isInZombieMode = true;
    }

    WriteInfo(
        TraceType,
        Root.TraceId,
        "A stop of node {0} {1}:{2} is in zombie mode = '{3}'",
        nodeConfig_->InstanceName,
        nodeConfig_->NodeId,
        federation_.Instance.getInstanceId(),
        isInZombieMode);
    return ErrorCodeValue::Success;
}

ErrorCode TestabilitySubsystem::OnClose()
{
    federation_.UnRegisterMessageHandler(Actor::TestabilitySubsystem);
    queryJobQueue_->Close();
    unreliableTx_->Close();
    return ErrorCodeValue::Success;
}

void TestabilitySubsystem::OnAbort()
{
    federation_.UnRegisterMessageHandler(Actor::TestabilitySubsystem);
    queryJobQueue_->Close();
    unreliableTx_->Close();
}

void TestabilitySubsystem::RequestMessageHandler(
    MessageUPtr& message,
    RequestReceiverContextUPtr& requestReceiverContext)
{
    auto timeout = TimeoutHeader::FromMessage(*message).Timeout;
    auto activityId = FabricActivityHeader::FromMessage(*message).ActivityId;

    WriteNoise(
        TraceType,
        Root.TraceId,
        "Received message {0} at TestabilitySubSystem",
        activityId);

    BeginProcessRequest(
        move(message),
        move(requestReceiverContext),
        timeout,
        [this, activityId](AsyncOperationSPtr const & asyncOperation) mutable
    {
        this->OnProcessRequestComplete(activityId, asyncOperation);
    },
        this->Root.CreateAsyncOperationRoot());
}

void TestabilitySubsystem::OnProcessRequestComplete(
    ActivityId const & activityId,
    AsyncOperationSPtr const & asyncOperation)
{
    MessageUPtr reply;
    RequestReceiverContextUPtr requestContext;
    auto error = EndProcessRequest(asyncOperation, reply, requestContext);

    if (error.IsSuccess())
    {
        WriteNoise(
            TraceType,
            Root.TraceId,
            "Query Request with ActivityId {0} Succeeded",
            activityId);

        reply->Headers.Replace(FabricActivityHeader(activityId));
        requestContext->Reply(std::move(reply));
    }
    else
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "Query Request with ActivityId {0} failed with {1}",
            activityId,
            error);

        requestContext->Reject(error);
    }
}

ErrorCode TestabilitySubsystem::EndProcessRequest(
    AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply,
    __out Federation::RequestReceiverContextUPtr & requestContext)
{
    return TestabilitySubsystemQueryMessageHandler::End(
        asyncOperation,
        reply,
        requestContext);
}

AsyncOperationSPtr TestabilitySubsystem::BeginProcessRequest(
    MessageUPtr && message,
    RequestReceiverContextUPtr && receiverContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IF(message->Action != QueryTcpMessage::QueryAction, "Error The requested action {0} is not supported in TestabilitySubsystem", message->Action);

    return AsyncOperation::CreateAndStart<TestabilitySubsystemQueryMessageHandler>(
        *this,
        queryMessageHandler_,
        move(message),
        move(receiverContext),
        timeout,
        callback,
        parent);
}

ErrorCode TestabilitySubsystem::AddUnreliableTransportBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    return unreliableTx_->AddUnreliableTransportBehavior(queryArgument, activityId, queryResult);
}

ErrorCode TestabilitySubsystem::RemoveUnreliableTransportBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    return unreliableTx_->RemoveUnreliableTransportBehavior(queryArgument, activityId, queryResult);
}

ErrorCode TestabilitySubsystem::GetTransportBehaviors(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    return unreliableTx_->GetTransportBehaviors(queryArgument, activityId, queryResult);
}

ErrorCode TestabilitySubsystem::AddUnreliableLeaseBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    return unreliableLease_->AddUnreliableLeaseBehavior(queryArgument, activityId, queryResult);
}

ErrorCode TestabilitySubsystem::RemoveUnreliableLeaseBehavior(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    return unreliableLease_->RemoveUnreliableLeaseBehavior(queryArgument, activityId, queryResult);
}

ErrorCode TestabilitySubsystem::StopNode(
    QueryArgumentMap const & queryArgument,
    ActivityId const & activityId,
    __out QueryResult & queryResult)
{
    UNREFERENCED_PARAMETER(queryResult);

    wstring instanceId;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Node::InstanceId, instanceId) ||
        instanceId.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring restartNodeString;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Node::Restart, restartNodeString) ||
        restartNodeString.empty())
    {
        return ErrorCodeValue::InvalidArgument;
    }

    wstring createFabricDumpString;
    if (!queryArgument.TryGetValue(QueryResourceProperties::Node::CreateFabricDump, createFabricDumpString) ||
        createFabricDumpString.empty())
    {
        // Setting the createFabricDumpString to false if empty for backward compatibility with older clients. 
        createFabricDumpString = L"false";
    }

    WriteInfo(
           TraceType,
           Root.TraceId,
           "{0} - StartStop parsing duration from query arg to string",
           activityId);

    wstring durationString;    
    if (!queryArgument.TryGetValue(QueryResourceProperties::Node::StopDurationInSeconds, durationString) ||
            instanceId.empty())
    {
        // Not specified, use default which is infinite
        durationString = L"4294967295"; // max DWORD
    }

    uint64 nodeInstanceId;
    if (!StringUtility::TryFromWString<uint64>(instanceId, nodeInstanceId))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    bool restartNode;
    if (!StringUtility::TryFromWString<bool>(restartNodeString, restartNode))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    WStringLiteral commandLabel = restartNode ? ServiceModel::Constants::RestartNodeCommand : ServiceModel::Constants::StopNodeCommand;

    bool createFabricDump;
    if (!StringUtility::TryFromWString<bool>(createFabricDumpString, createFabricDump))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    DWORD duration;
    if (!StringUtility::TryFromWString<DWORD>(durationString, duration))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    WriteInfo(
        TraceType,
        Root.TraceId,
        "{0} - StartStop restartNodeString:{1} restartNode:{2} createFabricDump :{3}, duration:{4}",
        activityId,
        restartNodeString,
        restartNode,
        createFabricDump,
        duration);

    if (nodeInstanceId != 0 && federation_.Instance.getInstanceId() != nodeInstanceId)
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0} - Failing {1}[{2}] the node instance did not match. Expected {3}",
            activityId,
            commandLabel,
            queryArgument,
            federation_.Instance.getInstanceId());

        return ErrorCodeValue::InstanceIdMismatch;
    }

    if (!restartNode)
    {
        wstring workingDir = nodeConfig_->WorkingDir;
        wstring startStopFileName = nodeConfig_->StartStopFileName;
        wstring absolutePathToStartStopFile = Path::Combine(workingDir, startStopFileName);
        wstring lockFile = wformatString("{0}.lock", absolutePathToStartStopFile);
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0} - StartStop file name is:{1}, lockFile is:{2}",
            activityId,
            absolutePathToStartStopFile,
            lockFile);
        {
            ExclusiveFile fileLock(lockFile);
            auto err = fileLock.Acquire(TimeSpan::FromSeconds(10));
            if (!err.IsSuccess())
            {
                WriteInfo(
                    TraceType,
                    Root.TraceId,
                    "Failed to acquire StartStop lock file {0}. Associated file={1}, error ={2}",
                    lockFile,
                    absolutePathToStartStopFile,
                    err);

                return ErrorCode(ErrorCodeValue::Timeout);
            }

            if (File::Exists(absolutePathToStartStopFile))
            {
                WriteInfo(
                    TraceType,
                    Root.TraceId,
                    "A stop of node {0} {1}:{2} is already in progress, returning success",
                    nodeConfig_->InstanceName,
                    nodeConfig_->NodeId,
                    federation_.Instance.getInstanceId());

                return ErrorCode();
            }

            WriteInfo(
                TraceType,
                Root.TraceId,
                "StopNode writing instance id {0}",
                federation_.Instance.getInstanceId());

#if !defined(PLATFORM_UNIX)
            std::wofstream startStopFile(absolutePathToStartStopFile);
#else
            string absolutePathToStartStopFileUtf8;
            StringUtility::Utf16ToUtf8(absolutePathToStartStopFile, absolutePathToStartStopFileUtf8);
            std::ofstream startStopFile(absolutePathToStartStopFileUtf8);
#endif

            if (duration == 4294967295)
            {
                startStopFile << federation_.Instance.getInstanceId();
            }
            else
            {
                WriteInfo(
                    TraceType,
                    Root.TraceId,
                    "StopNode writing instance id {0} AND duration '{1}'",
                    federation_.Instance.getInstanceId(),
                    duration);
                
                    DateTime expiredTime = DateTime::Now() + TimeSpan::FromSeconds(duration);
                    wstring expiredTimeAsString = expiredTime.ToString();
                    WriteInfo(
                        TraceType,
                        Root.TraceId,
                        "StopNode writing instance id {0} AND duration (as datetime string)  '{1}'",
                        federation_.Instance.getInstanceId(),
                        expiredTimeAsString);
#if !defined(PLATFORM_UNIX)
                    startStopFile << federation_.Instance.getInstanceId() << L" " << expiredTimeAsString;
#else
                    startStopFile << federation_.Instance.getInstanceId() << " " << formatString(expiredTime);
#endif
            }
            
            startStopFile.close();
        }
    }

    TimeSpan exitDelay = TimeSpan::FromSeconds(5);
    WriteInfo(
        TraceType,
        Root.TraceId,
        "{0} - Executing {1}[{2}] in {3} seconds",
        activityId,
        commandLabel,
        queryArgument,
        exitDelay.TotalSeconds());

    auto componentRoot = Root.CreateComponentRoot();
    Threadpool::Post(
        [this, componentRoot, activityId, commandLabel, createFabricDump]() mutable
    {
        WriteInfo(
            TraceType,
            Root.TraceId,
            "{0}: Killing node on command {1}",
            activityId,
            commandLabel);

        ASSERT_IF(createFabricDump == true, "This Fabric.exe dump was intentionally created by the user. Please ignore any alert(s) raised by this");
        ExitProcess(ErrorCodeValue::ProcessDeactivated & 0x0000FFFF);
    }, exitDelay);

    return ErrorCode();
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Query;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;
using namespace Management::FaultAnalysisService;

StringLiteral const TraceType("FaultsHandler");

FaultsHandler::FaultsHandler(
    HttpGatewayImpl & server)
    : RequestHandlerBase(server)
{
}

FaultsHandler::~FaultsHandler()
{
}

ErrorCode FaultsHandler::Initialize()
{
    validHandlerUris.push_back(HandlerUriTemplate(
        MAKE_SUFFIX_PATH(Constants::FaultsEntitySetPath, Constants::Cancel),
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(CancelTestCommand)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::FaultsEntitySetPath,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetTestCommandsList)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::StartDataLoss,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionDataLossHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::StartQuorumLoss,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionQuorumLossHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::StartRestart,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartPartitionRestartHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::GetDataLossProgress,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetDataLossProgressHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::GetQuorumLossProgress,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetQuorumLossProgressHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::GetRestartProgress,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetPartitionRestartProgressHandler)));


    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::StartNodeTransition,
        Constants::HttpPostVerb,
        MAKE_HANDLER_CALLBACK(StartNodeTransitionHandler)));

    validHandlerUris.push_back(HandlerUriTemplate(
        Constants::StartNodeTransitionProgress,
        Constants::HttpGetVerb,
        MAKE_HANDLER_CALLBACK(GetNodeTransitionProgressHandler)));

    return server_.InnerServer->RegisterHandler(Constants::FaultsHandlerPath, shared_from_this());
}

void FaultsHandler::StartPartitionDataLossHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    InvokeDataLossDescription description;
    auto error = GetDataLossDescription(argumentParser, description);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    
    auto inner = client.TestMgmtClient->BeginInvokeDataLoss(
        description,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStartPartitionDataLossComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    
    OnStartPartitionDataLossComplete(inner, true);
}

void FaultsHandler::OnStartPartitionDataLossComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndInvokeDataLoss(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void FaultsHandler::StartPartitionQuorumLossHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    InvokeQuorumLossDescription description;
    auto error = GetQuorumLossDescription(argumentParser, description);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
         
    auto inner = client.TestMgmtClient->BeginInvokeQuorumLoss(
        description,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStartPartitionQuorumLossComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStartPartitionQuorumLossComplete(inner, true);
}

void FaultsHandler::OnStartPartitionQuorumLossComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndInvokeQuorumLoss(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void FaultsHandler::StartPartitionRestartHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    RestartPartitionDescription description;
    auto error = GetPartitionRestartDescription(argumentParser, description);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }
    
    auto inner = client.TestMgmtClient->BeginRestartPartition(
        description,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStartPartitionRestartComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnStartPartitionRestartComplete(inner, true);
}

void FaultsHandler::OnStartPartitionRestartComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndRestartPartition(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void FaultsHandler::GetDataLossProgressHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);    

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid operationId = Guid::Empty();
    auto error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginGetInvokeDataLossProgress(
        operationId,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetProgressComplete(operation, false, FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS);
        },
        handlerOperation->shared_from_this());             
        
    OnGetProgressComplete(inner, true, FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS);
}

void FaultsHandler::GetQuorumLossProgressHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);    

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid operationId = Guid::Empty();
    auto error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginGetInvokeQuorumLossProgress(
        operationId,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetProgressComplete(operation, false, FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS);
        },
        handlerOperation->shared_from_this());             
        
    OnGetProgressComplete(inner, true, FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS);
}

void FaultsHandler::GetPartitionRestartProgressHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);    

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid operationId = Guid::Empty();
    auto error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginGetRestartPartitionProgress(
        operationId,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetProgressComplete(operation, false, FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION);
        },
        handlerOperation->shared_from_this());             
        
    OnGetProgressComplete(inner, true, FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION);
}

void FaultsHandler::StartNodeTransitionHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    StartNodeTransitionDescription description;
    auto error = GetStartNodeTransitionDescription(argumentParser, description);
    if (!error.IsSuccess())
    {
       handlerOperation->OnError(thisSPtr, error);
       return;
    }

    auto inner = client.TestMgmtClient->BeginStartNodeTransition(
        description,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnStartNodeTransitionComplete(operation, false);
        },
        handlerOperation->shared_from_this());
    
    OnStartNodeTransitionComplete(inner, true);
}

void FaultsHandler::OnStartNodeTransitionComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;
    auto error = client.TestMgmtClient->EndStartNodeTransition(operation);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody), Constants::StatusAccepted, *Constants::StatusDescriptionAccepted);
}

void FaultsHandler::GetNodeTransitionProgressHandler(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);    

    auto &client = handlerOperation->FabricClient;
    UriArgumentParser argumentParser(handlerOperation->Uri);

    Guid operationId = Guid::Empty();
    auto error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    auto inner = client.TestMgmtClient->BeginGetNodeTransitionProgress(
        operationId,        
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetProgressComplete(operation, false, FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION);
        },
        handlerOperation->shared_from_this());             
        
    OnGetProgressComplete(inner, true, FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION);
}

void FaultsHandler::OnGetProgressComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously,
    __in DWORD commandType)
{
    WriteInfo(TraceType, "Enter OnGetProgressComplete, commandType='{0}'", commandType);

    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    ErrorCode error = ErrorCodeValue::InvalidOperation;
    ByteBufferUPtr bufferUPtr = nullptr;

    if (commandType == FABRIC_TEST_COMMAND_TYPE_INVOKE_DATA_LOSS)
    {
        IInvokeDataLossProgressResultPtr  dataLossProgressResultPtr;

        error = client.TestMgmtClient->EndGetInvokeDataLossProgress(operation, dataLossProgressResultPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }

        InvokeDataLossProgress progress;
        progress.FromInternalInterface(dataLossProgressResultPtr);
        error = handlerOperation->Serialize(progress, bufferUPtr);
    }
    else if (commandType == FABRIC_TEST_COMMAND_TYPE_INVOKE_QUORUM_LOSS)
    {
        IInvokeQuorumLossProgressResultPtr  quorumLossProgressResultPtr;

        error = client.TestMgmtClient->EndGetInvokeQuorumLossProgress(operation, quorumLossProgressResultPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }

        InvokeQuorumLossProgress progress; 
        progress.FromInternalInterface(quorumLossProgressResultPtr);
        error =  handlerOperation->Serialize(progress, bufferUPtr);
    }
    else if (commandType == FABRIC_TEST_COMMAND_TYPE_INVOKE_RESTART_PARTITION)
    {
        IRestartPartitionProgressResultPtr restartPartitionProgressResultPtr;

        error = client.TestMgmtClient->EndGetRestartPartitionProgress(operation, restartPartitionProgressResultPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }

        RestartPartitionProgress progress;

        progress.FromInternalInterface(restartPartitionProgressResultPtr);
        error = handlerOperation->Serialize(progress, bufferUPtr);
    }
    else if (commandType == FABRIC_TEST_COMMAND_TYPE_START_NODE_TRANSITION)
    {
        INodeTransitionProgressResultPtr progressResultPtr;
    
        error = client.TestMgmtClient->EndGetNodeTransitionProgress(operation, progressResultPtr);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(operation->Parent, error);
            return;
        }
    
        HttpGateway::NodeTransitionProgress progress;
    
        progress.FromInternalInterface(progressResultPtr);
        error = handlerOperation->Serialize(progress, bufferUPtr);
    }

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void FaultsHandler::GetTestCommandsList(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;              
  
    wstring stateFilterString;
    DWORD stateFilter = 0;
    if (handlerOperation->Uri.GetItem(Constants::StateFilterString, stateFilterString))
    {
        auto error = Utility::TryParseQueryFilter(stateFilterString, 0, stateFilter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    wstring typeFilterString;
    DWORD typeFilter = 0;
    if (handlerOperation->Uri.GetItem(Constants::TypeFilterString, typeFilterString))
    {
        auto error = Utility::TryParseQueryFilter(typeFilterString, 0, typeFilter);
        if (!error.IsSuccess())
        {
            handlerOperation->OnError(thisSPtr, error);
            return;
        }
    }

    auto inner = client.TestMgmtClient->BeginGetTestCommandStatusList(
        stateFilter,        
        typeFilter,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnGetTestCommandStatusListComplete(operation, false);
        },
        handlerOperation->shared_from_this());
        
    OnGetTestCommandStatusListComplete(inner, true);
}

void FaultsHandler::OnGetTestCommandStatusListComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    vector<TestCommandListQueryResult> result;

    auto error = client.TestMgmtClient->EndGetTestCommandStatusList(operation, result);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr bufferUPtr;

    error = handlerOperation->Serialize(result, bufferUPtr);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    handlerOperation->OnSuccess(operation->Parent, move(bufferUPtr));
}

void FaultsHandler::CancelTestCommand(__in AsyncOperationSPtr const& thisSPtr)
{
    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(thisSPtr);
    auto &client = handlerOperation->FabricClient;
        
    UriArgumentParser argumentParser(handlerOperation->Uri);
    Guid operationId;
    auto error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    bool force = false;
    error = argumentParser.TryGetForce(force);
    if (!error.IsSuccess())
    {
        handlerOperation->OnError(thisSPtr, error);
        return;
    }

    CancelTestCommandDescription description(
        operationId, 
        force); 

    auto inner = client.TestMgmtClient->BeginCancelTestCommand(
        description,
        handlerOperation->Timeout,
        [this](AsyncOperationSPtr const& operation)
        {
            this->OnCancelTestCommandComplete(operation, false);
        },
        handlerOperation->shared_from_this());

    OnCancelTestCommandComplete(inner, true);
}

void FaultsHandler::OnCancelTestCommandComplete(
    __in AsyncOperationSPtr const& operation,
    __in bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    auto handlerOperation = AsyncOperation::Get<HandlerAsyncOperation>(operation->Parent);
    auto &client = handlerOperation->FabricClient;

    auto error = client.TestMgmtClient->EndCancelTestCommand(operation);

    if (!error.IsSuccess())
    {
        handlerOperation->OnError(operation->Parent, error);
        return;
    }

    ByteBufferUPtr emptyBody;
    handlerOperation->OnSuccess(operation->Parent, move(emptyBody));
}

ErrorCode FaultsHandler::TryGetCommonFields(
    __in UriArgumentParser & argumentParser,
    __out std::unique_ptr<PartitionCommonData> & partitionCommonData)
{
    NamingUri serviceNameUri;
    auto error = argumentParser.TryGetServiceName(serviceNameUri);
    if (!error.IsSuccess())
    {    
        WriteWarning(TraceType, "Could not parse serviceName");
        return error;
    }
            
    Guid partitionId = Guid::Empty();
    error = argumentParser.TryGetRequiredPartitionId(partitionId);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse partitionId");
        return error;
    }
        
    Guid operationId = Guid::Empty();
    error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parser OperationId");
        return error;
    }

    auto partitionSelector = make_shared<PartitionSelector>(
        move(serviceNameUri),
        PartitionSelectorType::PartitionId, // always must be this for this rest api
        partitionId.ToString());

    auto temp = make_unique<PartitionCommonData>(operationId, partitionSelector);
    partitionCommonData = move(temp);

    return ErrorCodeValue::Success;
}

ErrorCode FaultsHandler::GetDataLossDescription(__in UriArgumentParser & argumentParser, __out InvokeDataLossDescription & description)
{
    unique_ptr<PartitionCommonData> partitionCommonData;
    auto error = TryGetCommonFields(argumentParser, partitionCommonData);
    if (!error.IsSuccess())
    {        
        return error;
    }
        
    DataLossMode::Enum mode = DataLossMode::Enum::Invalid;
    error = argumentParser.TryGetDataLossMode(mode);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse DataLossMode");
        return error;
    }    

    InvokeDataLossDescription d(partitionCommonData->OperationId, partitionCommonData->PartitionSelector, mode);
    description = move(d);

    return ErrorCodeValue::Success;
}

ErrorCode FaultsHandler::GetQuorumLossDescription(__in UriArgumentParser & argumentParser, __out InvokeQuorumLossDescription & description)
{
    unique_ptr<PartitionCommonData> partitionCommonData;
    auto error = TryGetCommonFields(argumentParser, partitionCommonData);
    if (!error.IsSuccess())
    {        
        return error;
    }
        
    QuorumLossMode::Enum mode = QuorumLossMode::Enum::Invalid;
    error = argumentParser.TryGetQuorumLossMode(mode);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse QuorumLossMode");
        return error;
    }    

    DWORD quorumLossDurationInSeconds = 0;
    error = argumentParser.TryGetQuorumLossDuration(quorumLossDurationInSeconds);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse QuorumLossDuration");
        return error;
    }

    TimeSpan duration = TimeSpan::FromSeconds(quorumLossDurationInSeconds);

    InvokeQuorumLossDescription d(partitionCommonData->OperationId, partitionCommonData->PartitionSelector, mode, duration);
    description = move(d);
    return ErrorCodeValue::Success;
}

ErrorCode FaultsHandler::GetPartitionRestartDescription(__in UriArgumentParser & argumentParser, __out RestartPartitionDescription & description)
{
    unique_ptr<PartitionCommonData> partitionCommonData;
    auto error = TryGetCommonFields(argumentParser, partitionCommonData);
    if (!error.IsSuccess())
    {        
        return error;
    }
        
    RestartPartitionMode::Enum mode = RestartPartitionMode::Enum::Invalid;
    error = argumentParser.TryGetRestartPartitionMode(mode);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse RestartPartitionMode");
        return error;
    }    

    RestartPartitionDescription d(partitionCommonData->OperationId, partitionCommonData->PartitionSelector, mode);
    description = move(d);

    return ErrorCodeValue::Success;
}

ErrorCode FaultsHandler::GetStartNodeTransitionDescription(
    __in UriArgumentParser & argumentParser, 
    __out StartNodeTransitionDescription & description)
{
    wstring nodeName;
    auto error = argumentParser.TryGetNodeName(nodeName);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse nodeName");
        return error;
    }
      
    Guid operationId = Guid::Empty();
    error = argumentParser.TryGetOperationId(operationId);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse OperationId");
        return error;
    }

    uint64 nodeInstanceId = 0;
    error = argumentParser.TryGetNodeInstanceId(nodeInstanceId);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse NodeInstanceId");
        return error;
    }

    if (nodeInstanceId == 0)
    {
        WriteWarning(TraceType, "NodeInstanceId cannot be 0");
        return ErrorCodeValue::InvalidInstanceId;
    }

    FABRIC_NODE_TRANSITION_TYPE nodeTransitionType = FABRIC_NODE_TRANSITION_TYPE_INVALID;
    error = argumentParser.TryGetNodeTransitionType(nodeTransitionType);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, "Could not parse node transition type");
        return error;
    }

    DWORD stopDurationInSeconds = 0;     
    if (nodeTransitionType == FABRIC_NODE_TRANSITION_TYPE_STOP)
    {    
        error = argumentParser.TryGetStopDurationInSeconds(stopDurationInSeconds);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, "Could not parse node transition type");
            return error;
        }
    }
        
    StartNodeTransitionDescription d(
        nodeTransitionType,
        operationId,
        nodeName,
        nodeInstanceId,
        stopDurationInSeconds);
    description = move(d);
        
    return ErrorCodeValue::Success;
}


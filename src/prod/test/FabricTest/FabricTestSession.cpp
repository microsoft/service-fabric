// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace std;
using namespace Transport;

const StringLiteral FabricTestSession::traceSource_ = "FabricTestConfigSource";
const StringLiteral FabricTestSession::FabricTestHelpFileName = "FabricTestHelp.txt";

FabricTestSessionSPtr * FabricTestSession::singleton_ = nullptr;

void FabricTestSession::AddCommand(std::wstring const & command)
{
    TestCommon::TestSession::AddInput(Command::CreateCommand(command));
}

FabricTestSession::FabricTestSession(wstring const& label, bool autoMode, wstring const& serverListenAddress) 
    : fabricDispatcher_(),
    section_(L"FabricTest"),
    addressHelper_(),
    requestTable_(),
    ServerTestSession(
    serverListenAddress, 
    label, 
    FabricTestSessionConfig::GetConfig().MaxClientSessionIdleTimeout,
    autoMode, 
    fabricDispatcher_, 
    std::wstring(FabricTestHelpFileName.begin(), FabricTestHelpFileName.end()))
{
}

void FabricTestSession::CreateSingleton(wstring const& label, bool autoMode, wstring const& serverListenAddress)
{
    singleton_ = new shared_ptr<FabricTestSession>(new FabricTestSession(label, autoMode, serverListenAddress));
}

FabricTestSession & FabricTestSession::GetFabricTestSession()
{
    return *(*FabricTestSession::singleton_);
}

void FabricTestSession::OnClientDataReceived(wstring const& messageType, vector<byte> & data, wstring const& clientId)
{
    TestSession::WriteNoise(traceSource_, "OnClientDataReceived with message type {0} from {1}....", messageType, clientId);
    if (messageType == MessageType::ServicePlacementData)
    {
        ServicePlacementData servicePlacementData;
        FabricSerializer::Deserialize(servicePlacementData, data);
        NodeId nodeId;
        NodeId::TryParse(servicePlacementData.CodePackageContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientDataReceived for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessServicePlacementData(servicePlacementData);
    }
    else if (messageType == MessageType::CodePackageActivate)
    {
        TestCodePackageData codePackageData;
        FabricSerializer::Deserialize(codePackageData, data);
        NodeId nodeId;
        NodeId::TryParse(codePackageData.CodePackageContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientDataReceived for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessCodePackageHostInit(codePackageData.CodePackageContext);
        testNode->ProcessUpdateServiceManifest(codePackageData);
    }
    else if (messageType == MessageType::ServiceManifestUpdate)
    {
        TestCodePackageData codePackageData;
        FabricSerializer::Deserialize(codePackageData, data);
        NodeId nodeId;
        NodeId::TryParse(codePackageData.CodePackageContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientDataReceived for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessUpdateServiceManifest(codePackageData);
    }
    else if (messageType == MessageType::ClientCommandReply)
    {
        ClientCommandReply clientCommandReply;
        FabricSerializer::Deserialize(clientCommandReply, data);
        // In order to reuse request table I have to use message. 
        // Consider making request table generic
        Transport::MessageUPtr message(make_unique<Transport::Message>(clientCommandReply));
        message->Headers.Add<RelatesToHeader>(RelatesToHeader(clientCommandReply.MessageId));
        requestTable_.OnReplyMessage(*message);
    }
    else
    {
        TestSession::FailTest("MessageType not supported {0}", messageType);
    }
}

void FabricTestSession::OnClientConnection(wstring const& clientId)
{
    TestSession::WriteNoise(traceSource_, "OnClientConnection from {0}....", clientId);
    TestMultiPackageHostContext testMultiPackageHostContext;
    if(TestMultiPackageHostContext::FromClientId(clientId, testMultiPackageHostContext))
    {
        NodeId nodeId;
        NodeId::TryParse(testMultiPackageHostContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientConnection for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessMultiCodePackageHostInit(testMultiPackageHostContext);
    }
    else
    {
        TestCodePackageContext testCodePackageContext;
        TestSession::FailTestIfNot(TestCodePackageContext::FromClientId(clientId, testCodePackageContext), "Could not parse client id {0}", clientId);
        NodeId nodeId;
        NodeId::TryParse(testCodePackageContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientConnection for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessCodePackageHostInit(testCodePackageContext);
    }
}

void FabricTestSession::OnClientFailure(wstring const& clientId)
{
    TestSession::WriteNoise(traceSource_, "OnClientFailure for {0}....", clientId);
    if(!FabricDispatcher.IsFabricActivationManagerOpen())
    {
        TestSession::WriteNoise(traceSource_, "FabricActivationManager is closing/closed. Ignore call for failure of clientId {0}", clientId);
        return;
    }

    TestMultiPackageHostContext testMultiPackageHostContext;
    if(TestMultiPackageHostContext::FromClientId(clientId, testMultiPackageHostContext))
    {
        NodeId nodeId;
        NodeId::TryParse(testMultiPackageHostContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientFailure for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessMultiCodePackageHostFailure(testMultiPackageHostContext);
    }
    else
    {
        TestCodePackageContext testCodePackageContext;
        TestSession::FailTestIfNot(TestCodePackageContext::FromClientId(clientId, testCodePackageContext), "Could not parse client id {0}", clientId);
        NodeId nodeId;
        NodeId::TryParse(testCodePackageContext.NodeId, nodeId);
        FabricTestNodeSPtr testNode = FabricDispatcher.Federation.GetNode(nodeId);
        if (!testNode)
        {
            TestSession::WriteNoise(traceSource_, "OnClientFailure for invalid node {0} from {1}", nodeId, clientId);
            return;
        }

        testNode->ProcessCodePackageHostFailure(testCodePackageContext);
    }
}

AsyncOperationSPtr FabricTestSession::BeginSendClientCommandRequest(
    wstring const& command,
    Transport::MessageId messageIdInReply,
    wstring const & clientId,
    Common::TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    auto requestAsyncOperation = make_shared<RequestAsyncOperation>(
        requestTable_,
        messageIdInReply,
        timeout,
        callback,
        parent);
    requestAsyncOperation->Start(requestAsyncOperation);

    auto error = this->requestTable_.TryInsertEntry(messageIdInReply, requestAsyncOperation);
    if (error.IsSuccess())
    {
        if (requestAsyncOperation->IsCompleted)
        {
            // If there was a race and we added to the table after OnComplete ran,
            // we must remove it again since it will not itself.
            this->requestTable_.TryRemoveEntry(messageIdInReply, requestAsyncOperation);
        }
        else
        {
            ExecuteCommandAtClient(command, clientId);
        }
    }
    else
    {
        requestAsyncOperation->TryComplete(requestAsyncOperation, error);
    }

    return requestAsyncOperation;
}

ErrorCode FabricTestSession::EndSendClientCommandRequest(AsyncOperationSPtr const & operation, __out wstring & reply)
{
    MessageUPtr messageReply;
    auto error =  RequestAsyncOperation::End(operation, messageReply);
    if(error.IsSuccess())
    {
        ClientCommandReply body;
        messageReply->GetBody(body);
        reply = body.Reply;
        error = ErrorCode(body.ErrorCodeValue);
    }

    return error;
}

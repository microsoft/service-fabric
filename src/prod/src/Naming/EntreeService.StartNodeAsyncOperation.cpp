// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace std;
    using namespace Common;
    using namespace Client;
    using namespace Transport;
    using namespace Federation;
    using namespace ServiceModel;

    StringLiteral const TraceComponent("ProcessRequest.StartNode");
    
    const std::wstring EntreeService::StartNodeAsyncOperation::TraceId = L"StartNode";
    const UINT StartNodeExitCode = (UINT)(ErrorCodeValue::ProcessDeactivated & 0x0000FFFF);
        
    EntreeService::StartNodeAsyncOperation::StartNodeAsyncOperation(
        __in GatewayProperties & properties,
        Transport::MessageUPtr && receivedMessage,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback, 
        Common::AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase(properties, move(receivedMessage), timeout, callback, parent)
      , clusterManagementClientPtr_()
    {
    }

    void EntreeService::StartNodeAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {  
        if (!ReceivedMessage->GetBody(startNodeRequestBody_))
        {
            this->SetReplyAndComplete(
                thisSPtr, 
                NamingMessage::GetStartNodeReply(), 
                ErrorCodeValue::InvalidMessage);
            return;
        }
      
        WriteInfo(
            TraceComponent,
            "{0}: StartNodeAsyncOperation, zombie mode:{1}, nodeName on 'this' node:{2}, nodeName in received message:{3}",
            TraceId,
            Properties.IsInZombieMode,
            Properties.NodeName,
            startNodeRequestBody_.NodeName);
        
        bool nodeIsInZombieMode = Properties.IsInZombieMode;
        bool messageIsForCurrentNode = Properties.NodeName == startNodeRequestBody_.NodeName;

        if (!nodeIsInZombieMode)
        {                       
            if (messageIsForCurrentNode)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: StartNode, node is not in zombie, message is for this node, node instance id: {1}, instance id in request {2}",
                    TraceId,
                    Properties.Instance.ToString(),
                    startNodeRequestBody_.InstanceId);    

                ErrorCode error = ErrorCodeValue::InstanceIdMismatch;                      

                if (File::Exists(Properties.AbsolutePathToStartStopFile))
                {
                    WriteInfo(
                        TraceComponent,
                        "{0}: StartNode, stop start file is already present, returning in progress",
                        TraceId);
                    error = ErrorCodeValue::StopInProgress;
                }

                else if (Properties.Instance.InstanceId > startNodeRequestBody_.InstanceId)
                {
                    error = ErrorCodeValue::NodeIsUp;
                }
                else if ((Properties.Instance.InstanceId == startNodeRequestBody_.InstanceId) || (startNodeRequestBody_.InstanceId == 0))
                {            
                    // Node is still up
                    WriteInfo(
                        TraceComponent,
                        "{0}: StartNode, node has not stopped yet - there is a pending StopNode",
                        TraceId);    
                    error = ErrorCodeValue::NodeHasNotStoppedYet;
                }
                
                this->SetReplyAndComplete(
                    thisSPtr, 
                    NamingMessage::GetStartNodeReply(), 
                    error);
                return;
            }
            else // !messageIsForCurrentNode
            {
                // If this node is not in zombine mode AND it's not for this node, try to forward it
                ForwardToNode(thisSPtr);
            }
        }
        else // InZombieMode
        {                           
            if (messageIsForCurrentNode)
            {
                if (startNodeRequestBody_.InstanceId != 0)  // 0 means the user does not care about a specific instance
                {                    
                    ErrorCode fileError = IncomingInstanceIdMatchesFile(startNodeRequestBody_);
                    if (!fileError.IsSuccess())
                    {
                        ErrorCodeValue::Enum inner = fileError.ReadValue();
                        if (inner == ErrorCodeValue::FileNotFound)
                        {
                            // This means another start just happened is in-flight - this node should go down soon, then come up as a normal node.
                            inner = ErrorCodeValue::Success;
                        }
                        
                        this->SetReplyAndComplete(
                            thisSPtr, 
                            NamingMessage::GetStartNodeReply(), 
                            inner);
                        return;
                    }                  
                }

                // Delete the file
                WriteInfo(
                    TraceComponent,
                    "{0}: StartStop file name is {1}.",
                    TraceId,
                    Properties.AbsolutePathToStartStopFile);

                bool fileExists = File::Exists(Properties.AbsolutePathToStartStopFile);
                if (fileExists)
                {
                    ErrorCode error = File::Delete2(Properties.AbsolutePathToStartStopFile);
                    if (!error.IsSuccess()) 
                    { 
                        WriteWarning(
                            TraceComponent,
                            "{0}: Deleting startstop file failed with {1}.",
                            TraceId,
                            error);
                        this->SetReplyAndComplete(
                            thisSPtr, 
                            NamingMessage::GetStartNodeReply(), 
                            error);
                        return;
                    }
                }            

                TimeSpan exitDelay = TimeSpan::FromSeconds(5);
                WriteInfo(
                    TraceComponent,
                    "{0}: restarting process in {1} seconds.",
                    TraceId,
                    exitDelay.TotalSeconds());

                this->SetReplyAndComplete(
                    thisSPtr, 
                    NamingMessage::GetStartNodeReply(), 
                    ErrorCode::Success());

                auto componentRoot = Properties.Root.CreateComponentRoot();
                Threadpool::Post(
                    [this, componentRoot] () mutable 
                { 
                    WriteInfo(
                        TraceComponent,
                        "{0}: Killing node as part of StartNode command",
                        TraceId);
                    ExitProcess(StartNodeExitCode);
                }, exitDelay);
                return;
            }
            else // !messageIsForCurrentNode
            {
                Assert::CodingError(
                    "Restart node message for node {0} is received on the zombie node {1}",
                    startNodeRequestBody_.NodeName,
                    Properties.NodeName);
            }
        }
    }
     
    void EntreeService::StartNodeAsyncOperation::ForwardToNode(AsyncOperationSPtr const &thisSPtr)
    {
        if (startNodeRequestBody_.IpAddressOrFQDN.empty() || startNodeRequestBody_.ClusterConnectionPort == 0)
        {
            GetEndpointForZombieNode(thisSPtr);
        }
        else
        {
            MakeRequest(
                thisSPtr, 
                startNodeRequestBody_.IpAddressOrFQDN, 
                startNodeRequestBody_.ClusterConnectionPort);
        }
    }

    void EntreeService::StartNodeAsyncOperation::MakeRequest(
        AsyncOperationSPtr const& thisSPtr,
        std::wstring const &ipAddressOrFQDN,
        ULONG port)
    {
        auto error = InitializeClient(ipAddressOrFQDN, port);
        if (!error.IsSuccess())
        {
            this->SetReplyAndComplete(
                thisSPtr, 
                NamingMessage::GetStartNodeReply(), 
                error);
            return;
        }

        AsyncOperationSPtr inner = clusterManagementClientPtr_->BeginStartNode(
            startNodeRequestBody_.NodeName,
            startNodeRequestBody_.InstanceId,
            ipAddressOrFQDN,
            port,
            RemainingTime,
            [this] (AsyncOperationSPtr const& asyncOperation) 
            {
                OnRequestCompleted(asyncOperation, false);
            },
            thisSPtr);

        OnRequestCompleted(inner, true);
    }

    void EntreeService::StartNodeAsyncOperation::OnRequestCompleted(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }
    
        MessageUPtr adminClientReply;
        ErrorCode error = clusterManagementClientPtr_->EndStartNode(asyncOperation);

        if (error.IsSuccess())
        {
            WriteInfo(
                TraceComponent,
                "{0}: StartStop OnRequestCompleted, replying with error: {1}.",
                TraceId,
                error);
            this->SetReplyAndComplete(
                asyncOperation->Parent, 
                NamingMessage::GetStartNodeReply(), 
                ErrorCode::Success());
        }
        else
        {        
           WriteWarning(
               TraceComponent,
               "{0}: StartStop OnRequestCompleted, replying with error: {1}.",
               TraceId,
               error);

           this->SetReplyAndComplete(
               asyncOperation->Parent, 
               NamingMessage::GetStartNodeReply(), 
               error);
        }
    }

    Common::ErrorCode EntreeService::StartNodeAsyncOperation::InitializeClient(
        wstring const &ipAddressOrFQDN,
        ULONG port)
    {
        // Format the address and port together into a string   
        wstring addressTemp = TcpTransportUtility::ConstructAddressString(ipAddressOrFQDN, port);
        
        auto address = addressTemp.c_str();
        Api::IClientFactoryPtr clientFactoryPtr;

        ErrorCode error = ClientFactory::CreateClientFactory(1, &address, clientFactoryPtr);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: CreateClientFactory failed with {1}.",
                TraceId,
                error);
            return error; 
        }
    
        Api::IClientSettingsPtr clientSettingsPtr;
        error = clientFactoryPtr->CreateSettingsClient(clientSettingsPtr);
        if (!error.IsSuccess()) 
        { 
             WriteWarning(
                TraceComponent,
                "{0}: CreateSettingsClient failed with {1}.",
                TraceId,
                error);
            return error; 
        }
    
        auto securitySettingsCopy = Properties.ZombieModeFabricClientSecuritySettings;
        error = clientSettingsPtr->SetSecurity(move(securitySettingsCopy));
        if (!error.IsSuccess()) 
        {   
            WriteWarning(
                TraceComponent,
                "{0}: SetSecurity failed with {1}.",
                TraceId,
                error);
            return error; 
        }
    
        error = clientFactoryPtr->CreateClusterManagementClient(clusterManagementClientPtr_);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: CreateClusterManagementClient failed with {1}.",
                TraceId,
                error);            
            return error; 
        }

        return error;
    }
    
    void EntreeService::StartNodeAsyncOperation::GetEndpointForZombieNode(AsyncOperationSPtr const& thisSPtr)
    {
        WriteInfo(
            TraceComponent,
            "Querying endpoints for zombie node {0}",
            startNodeRequestBody_.NodeName);

        auto error = TryCreateQueryClient();
        if (!error.IsSuccess())
        {
            this->SetReplyAndComplete(
                thisSPtr, 
                NamingMessage::GetStartNodeReply(), 
                error);
            return;
        }

        auto inner = queryClientPtr_->BeginGetFMNodeList(
            startNodeRequestBody_.NodeName,
            L"", /*continuationToken*/
            RemainingTime,
            [this](AsyncOperationSPtr const& operation)
            {
                this->OnGetEndpointForZombieNodeComplete(operation, false);
            },
            thisSPtr);

        OnGetEndpointForZombieNodeComplete(inner, true);
    }

    void EntreeService::StartNodeAsyncOperation::OnGetEndpointForZombieNodeComplete(
        AsyncOperationSPtr const& asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        vector<NodeQueryResult> nodeResult;
        PagingStatusUPtr pagingStatus;
        auto error = queryClientPtr_->EndGetNodeList(asyncOperation, nodeResult, pagingStatus);
        if (!error.IsSuccess())
        {
            this->SetReplyAndComplete(
                asyncOperation->Parent, 
                NamingMessage::GetStartNodeReply(), 
                error);
            return;
        }

        if (nodeResult.size() != 1)
        {
            //
            // The node name specified was invalid.
            //
            this->SetReplyAndComplete(
                asyncOperation->Parent, 
                NamingMessage::GetStartNodeReply(), 
                ErrorCodeValue::NodeNotFound);
            return;
        }
        
        if (IsNodeRunning(nodeResult[0].NodeStatus))
        {
            WriteInfo(
                TraceComponent,
                "{0}: StartNode, target node '{1}' is '{2}'",
                TraceId,
                startNodeRequestBody_.NodeName,
                nodeResult[0].NodeStatus);

            if ((nodeResult[0].InstanceId == startNodeRequestBody_.InstanceId) || (startNodeRequestBody_.InstanceId == 0))
            {            
                // Node is still up
                WriteInfo(
                    TraceComponent,
                    "{0}: StartNode, node has not stopped yet - there is a pending StopNode",
                    TraceId);    
                error = ErrorCodeValue::NodeHasNotStoppedYet;
            }
            else if (nodeResult[0].InstanceId > startNodeRequestBody_.InstanceId)
            {
                WriteInfo(
                    TraceComponent,
                    "{0}: StartNode, target node's instance id '{1}' is greater the instance id in request '{2}'",
                    TraceId,
                    nodeResult[0].InstanceId,
                    startNodeRequestBody_.InstanceId);  
                error = ErrorCodeValue::NodeIsUp;
            }
            else
            {
                // this would be from the future, return InstanceIdMismatch
                error = ErrorCodeValue::InstanceIdMismatch;              
            }
            
            this->SetReplyAndComplete(
                asyncOperation->Parent, 
                NamingMessage::GetStartNodeReply(), 
                error);
            return;
        }

        WriteInfo(
            TraceComponent,
            "{0}: StartNode, node was not up, it is {1}",
            TraceId,
            nodeResult[0].NodeStatus);

        if (IsNodeInvalid(nodeResult[0].NodeStatus))
        {
            this->SetReplyAndComplete(
                asyncOperation->Parent, 
                NamingMessage::GetStartNodeReply(), 
                ErrorCodeValue::NodeNotFound);
            return;
        }
        
        MakeRequest(
            asyncOperation->Parent, 
            nodeResult[0].IPAddressOrFQDN, 
            nodeResult[0].ClusterConnectionPort);
    }

    Common::ErrorCode EntreeService::StartNodeAsyncOperation::TryCreateQueryClient()
    {
        Api::IClientFactoryPtr clientFactoryPtr;
        ErrorCode error = ClientFactory::CreateLocalClientFactory(Properties.NodeConfigSPtr, clientFactoryPtr);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: CreateLocalClientFactory failed with {1}.",
                TraceId,
                error);
            return error;
        }
    
        error = clientFactoryPtr->CreateQueryClient(queryClientPtr_);
        if (!error.IsSuccess()) 
        { 
            WriteWarning(
                TraceComponent,
                "{0}: CreateClusterManagementClient failed with {1}.",
                TraceId,
                error);            
        }

        return error; 
    }

    ErrorCode EntreeService::StartNodeAsyncOperation::IncomingInstanceIdMatchesFile(StartNodeRequestMessageBody const & body)
    {
        wstring lockFile = wformatString("{0}.lock", Properties.AbsolutePathToStartStopFile);        
        uint64 instanceIdFromFile = 0;
        {
            ExclusiveFile fileLock(lockFile);
            auto err = fileLock.Acquire(TimeSpan::FromSeconds(10));
            if (!err.IsSuccess())
            {
                WriteWarning(
                    TraceComponent,
                    "{0}: Failed to acquire lock file '{1}': {2}",
                    TraceId,
                    lockFile,
                    err);

                return ErrorCodeValue::Timeout;
            }

            std::string absolutePathToStartFile;
            if (!File::Exists(Properties.AbsolutePathToStartStopFile))
            {
                return ErrorCodeValue::FileNotFound;
            }
            
            StringUtility::Utf16ToUtf8(Properties.AbsolutePathToStartStopFile, absolutePathToStartFile);            
            
#if !defined(PLATFORM_UNIX)            
            std::wifstream startStopFile(absolutePathToStartFile);
#else
            std::ifstream startStopFile(absolutePathToStartFile);
#endif
            startStopFile >> instanceIdFromFile;
            startStopFile.close();            
        }

        WriteInfo(
            TraceComponent,
            "{0}: Instance id in file is: {1}, instance id in request is {2}",
            TraceId,
            instanceIdFromFile,
            body.InstanceId);      

        if (body.InstanceId == instanceIdFromFile)
        {
            return ErrorCodeValue::Success;
        }
        else
        {
            return ErrorCodeValue::InstanceIdMismatch;
        }
    }    
    
    bool EntreeService::StartNodeAsyncOperation::IsNodeRunning(FABRIC_QUERY_NODE_STATUS nodeStatus)
    {
        return ((nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_UP) 
            || (nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_ENABLING) 
            || (nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_DISABLING) 
            || (nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_DISABLED));
    }

    bool EntreeService::StartNodeAsyncOperation::IsNodeInvalid(FABRIC_QUERY_NODE_STATUS nodeStatus)
    {
        return ((nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_INVALID) 
            || (nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_UNKNOWN)
            || (nodeStatus == FABRIC_QUERY_NODE_STATUS::FABRIC_QUERY_NODE_STATUS_REMOVED));
    }
}


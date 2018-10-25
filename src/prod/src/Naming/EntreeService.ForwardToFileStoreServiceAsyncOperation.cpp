// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{   
    using namespace Common;
    using namespace Federation;
    using namespace Query;
    using namespace Reliability;
    using namespace std;
    using namespace SystemServices;
    using namespace Transport;
    using namespace Management::FileStoreService;
    using namespace ClientServerTransport;

    StringLiteral const TraceComponent("ProcessRequest.ForwardToFileStoreServiceAsyncOperation");

    EntreeService::ForwardToFileStoreServiceAsyncOperation::ForwardToFileStoreServiceAsyncOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
        , serviceName_()
        , partitionId_()
        , activityHeader_()
    {
    }    

    void EntreeService::ForwardToFileStoreServiceAsyncOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartResolve(thisSPtr);
    }

    void EntreeService::ForwardToFileStoreServiceAsyncOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);

        Transport::ForwardMessageHeader forwardMessageHeader;
        if (!ReceivedMessage->Headers.TryReadFirst(forwardMessageHeader))
        {
            WriteWarning(TraceComponent, "{0} ForwardMessageHeader missing: {1}", this->TraceId, *ReceivedMessage);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        
        NamingPropertyHeader propertyHeader;               
        if (ReceivedMessage->Headers.TryReadFirst(propertyHeader))
        {
            WriteNoise(
                TraceComponent, 
                "{0} read property header ({1})",
                this->TraceId, 
                propertyHeader);

            serviceName_ = propertyHeader.Name;
            partitionId_ = propertyHeader.PropertyName;
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }
        
        if (!ReceivedMessage->Headers.TryReadFirst(activityHeader_))
        {
            activityHeader_ = FabricActivityHeader(Guid::NewGuid());

            WriteWarning(
                TraceComponent, 
                "{0} message {1} missing activity header: generated = {2}", 
                this->TraceId, 
                *ReceivedMessage, 
                activityHeader_);
        }

        ServiceRoutingAgentHeader routingAgentHeader;
        bool hasRoutingAgentHeader = ReceivedMessage->Headers.TryReadFirst(routingAgentHeader);
        
        ReceivedMessage->Headers.RemoveAll();

        ReceivedMessage->Headers.Replace(ActorHeader(forwardMessageHeader.Actor));
        ReceivedMessage->Headers.Replace(ActionHeader(forwardMessageHeader.Action));
        ReceivedMessage->Headers.Replace(activityHeader_);

        if (hasRoutingAgentHeader)
        {
            ReceivedMessage->Headers.Replace(routingAgentHeader);        
        }

        this->StartResolve(thisSPtr);
    }

    void EntreeService::ForwardToFileStoreServiceAsyncOperation::StartResolve(Common::AsyncOperationSPtr const & thisSPtr)
    {
        auto batch = Common::make_unique<NamePropertyOperationBatch>(serviceName_);
        batch->AddGetPropertyOperation(partitionId_);

        MessageUPtr message = NamingTcpMessage::GetPropertyBatch(std::move(batch))->GetTcpMessage();
        message->Headers.Replace(activityHeader_);

        auto inner = AsyncOperation::CreateAndStart<PropertyBatchAsyncOperation>(
            this->Properties, 
            std::move(message), 
            RemainingTime, 
            [this] (AsyncOperationSPtr const & asyncOperation) 
            { 
                this->OnResolveRequestComplete(asyncOperation, false /*expectedCompletedSynchronously*/);
            }, 
            thisSPtr);       
        this->OnResolveRequestComplete(inner, true /*expectedCompletedSynchronously*/);
    }

    void EntreeService::ForwardToFileStoreServiceAsyncOperation::OnResolveRequestComplete(
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (asyncOperation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        ErrorCode error = RequestAsyncOperationBase::End(asyncOperation, /*out*/reply);

        if (error.IsSuccess() && reply.get() == nullptr)
        {
            TRACE_WARNING_AND_TESTASSERT(TraceComponent, "{0}: null reply message on success", this->TraceId);

            error = ErrorCodeValue::OperationFailed;
        }

        if (error.IsSuccess())
        {
            NamePropertyOperationBatchResult batchResult;
            if (!reply->GetBody(batchResult))
            {
                this->TryComplete(asyncOperation->Parent, ErrorCodeValue::OperationFailed);
                return;
            }

            if(batchResult.Properties.empty())
            {
                this->HandleRetryStart(asyncOperation->Parent);
                return;
            }        

            vector<BYTE> serializedData = batchResult.Properties.front().Bytes;
            Management::FileStoreService::FileStorePartitionInfo info;
            error = FabricSerializer::Deserialize(info, serializedData);
            ASSERT_IFNOT(error.IsSuccess(), "Deserization of ReplicaInfo object should succeed. Error:{0}", error);

            WriteInfo(
                TraceComponent,
                "{0}: Received address from naming - SequenceNumber: {1}, Address: {2}",
                this->TraceId,
                batchResult.Properties.front().Metadata.SequenceNumber,
                info.PrimaryLocation);

            SystemServiceLocation parsedLocation;
            if (SystemServiceLocation::TryParse(info.PrimaryLocation, parsedLocation))
            {
                auto instance = this->Properties.RequestInstance.GetNextInstance();

                this->ReceivedMessage->Headers.Replace(TimeoutHeader(this->RemainingTime));
                this->ReceivedMessage->Headers.Replace(parsedLocation.CreateFilterHeader());
                this->ReceivedMessage->Headers.Replace(GatewayRetryHeader(this->RetryCount));
                this->ReceivedMessage->Headers.Replace(RequestInstanceHeader(instance));

                NodeInstance const & node = parsedLocation.NodeInstance;

                this->RouteToNode(
                    this->ReceivedMessage->Clone(),
                    node.Id,
                    node.InstanceId,
                    true, // exact routing
                    asyncOperation->Parent);       
                return;
            }
            else
            {
                WriteWarning(
                    TraceComponent, 
                    "{0}: could not parse system service location: {1}",
                    this->TraceId,
                    info.PrimaryLocation);

                this->HandleRetryStart(asyncOperation->Parent);
                return;
            }
        }
        else if (this->IsRetryable(error))
        {
            WriteWarning(
                TraceComponent, 
                "{0}: could not get FileStoreService address from naming - retrying. error = {1}", 
                this->TraceId,
                error);                

            this->HandleRetryStart(asyncOperation->Parent);
            return;
        }
        
        this->TryComplete(asyncOperation->Parent, error);        
    }

    bool EntreeService::ForwardToFileStoreServiceAsyncOperation::IsRetryable(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
            // Internally retry on these errors when routing to system services
            //
            case ErrorCodeValue::ServiceOffline:
            case ErrorCodeValue::PartitionNotFound:
                return true;

            default:
                return NamingErrorCategories::IsRetryableAtGateway(error);
        }
    }
}

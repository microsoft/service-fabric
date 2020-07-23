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
    using namespace ClientServerTransport;
    using namespace ServiceModel;

    StringLiteral const TraceComponent("ProcessRequest.ForwardToServiceOperation");

    EntreeService::ForwardToServiceOperation::ForwardToServiceOperation(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
        , targetServiceName_()
        , targetCuid_(ConsistencyUnitId::Invalid())
        , action_()
        , secondaryLocations_()
    {
    }    

    void EntreeService::ForwardToServiceOperation::OnRetry(AsyncOperationSPtr const & thisSPtr)
    {
        this->StartResolve(thisSPtr);
    }

    void EntreeService::ForwardToServiceOperation::OnStartRequest(AsyncOperationSPtr const & thisSPtr)
    {
        TimedAsyncOperation::OnStart(thisSPtr);                

        Transport::ForwardMessageHeader forwardMessageHeader;
        if (!ReceivedMessage->Headers.TryReadFirst(forwardMessageHeader))
        {
            WriteWarning(TraceComponent, "{0} ForwardMessageHeader missing: {1}", this->TraceId, *ReceivedMessage);
            TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
            return;
        }

        Actor::Enum targetActor = forwardMessageHeader.Actor;

        ServiceRoutingAgentHeader routingAgentHeader;
        bool hasRoutingAgentHeader = ReceivedMessage->Headers.TryReadFirst(routingAgentHeader);

        if (hasRoutingAgentHeader)
        {
            targetActor = routingAgentHeader.Actor;
            action_ = routingAgentHeader.Action;
        }
        else
        {
            action_ = forwardMessageHeader.Action;
        }

        //
        // System services can be routed to in one of three ways:
        //
        // 1) Explicitly specifying the service name
        // 2) Explicitly specifying the partition ID
        // 3) By hardcoded partition ID
        //
        // 3) Is the legacy approach used by the CM to simplify resolution.
        // Dynamically created system services should prefer either 1) or 2).
        //

        ServiceTargetHeader serviceTargetHeader;
        PartitionTargetHeader partitionHeader;

        if (ReceivedMessage->Headers.TryReadFirst(serviceTargetHeader))
        {
            WriteNoise(
                TraceComponent, 
                "{0} read service target header ({1})",
                this->TraceId, 
                serviceTargetHeader.ServiceName);

            if (this->TryValidateServiceTarget(targetActor, serviceTargetHeader))
            {
                targetServiceName_ = serviceTargetHeader.ServiceName.ToString();
            }
            else
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidOperation);
                return;
            }
        }
        else if (ReceivedMessage->Headers.TryReadFirst(partitionHeader))
        {
            WriteNoise(
                TraceComponent, 
                "{0} read partition header ({1})",
                this->TraceId, 
                partitionHeader.PartitionId);

            targetCuid_ = ConsistencyUnitId(partitionHeader.PartitionId);
        }
        else
        {
            if (!this->TryInitializeStaticCuidTarget(targetActor))
            {
                this->TryComplete(thisSPtr, ErrorCodeValue::InvalidMessage);
                return;
            }
        }

        FabricActivityHeader activityHeader;
        if (!ReceivedMessage->Headers.TryReadFirst(activityHeader))
        {
            activityHeader = FabricActivityHeader(Guid::NewGuid());

            WriteWarning(
                TraceComponent, 
                "{0} message {1} missing activity header: generated = {2}", 
                this->TraceId, 
                *ReceivedMessage, 
                activityHeader);
        }

        QueryAddressHeader queryAddressHeader;
        bool hasQueryAddressHeader = ReceivedMessage->Headers.TryReadFirst(queryAddressHeader);

        CreateComposeDeploymentRequestHeader createComposeDeploymentRequestHeader;
        auto hasCreateContainerApplicationRequestHeader = ReceivedMessage->Headers.TryReadFirst(createComposeDeploymentRequestHeader);

        UpgradeComposeDeploymentRequestHeader upgradeComposeDeploymentRequestHeader;
        auto hasUpgradeComposeDeploymentRequestHeader = ReceivedMessage->Headers.TryReadFirst(upgradeComposeDeploymentRequestHeader);

        // v6.0 is sending UpgradeComposeDeploymentRequestCompatibilityHeader which was conflicted.
        UpgradeComposeDeploymentRequestCompatibilityHeader upgradeComposeDeploymentRequestCompatibilityHeader;
        auto hasUpgradeComposeDeploymentRequestCompatibilityHeader = ReceivedMessage->Headers.TryReadFirst(upgradeComposeDeploymentRequestCompatibilityHeader);

        ReceivedMessage->Headers.RemoveAll();

        ReceivedMessage->Headers.Add(ActorHeader(forwardMessageHeader.Actor));
        ReceivedMessage->Headers.Add(ActionHeader(forwardMessageHeader.Action));        
        ReceivedMessage->Headers.Add(activityHeader);

        if (hasQueryAddressHeader) 
        {
            ReceivedMessage->Headers.Add(queryAddressHeader);        
        }

        if (hasRoutingAgentHeader)
        {
            ReceivedMessage->Headers.Add(routingAgentHeader);        
        }

        if (hasCreateContainerApplicationRequestHeader)
        {
            ReceivedMessage->Headers.Add(createComposeDeploymentRequestHeader);
        }

        if (hasUpgradeComposeDeploymentRequestHeader)
        {
            ReceivedMessage->Headers.Add(upgradeComposeDeploymentRequestHeader);
        }

        if (hasUpgradeComposeDeploymentRequestCompatibilityHeader)
        {
            ReceivedMessage->Headers.Add(upgradeComposeDeploymentRequestCompatibilityHeader);
        }

        this->StartResolve(thisSPtr);
    }

    void EntreeService::ForwardToServiceOperation::StartResolve(Common::AsyncOperationSPtr const & thisSPtr)
    {
        AsyncOperationSPtr operation;

        if (targetCuid_.IsInvalid)
        {
            operation = this->Properties.SystemServiceResolver->BeginResolve(
                targetServiceName_,
                this->ActivityHeader.ActivityId,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
                thisSPtr);
        }
        else
        {
            ConsistencyUnitId c = ConsistencyUnitId(targetCuid_);

            // SystemServiceResolver handles both regular and fabSrvServices, new fabSrvServices need to be updated inside SystemServiceResolver

            operation = this->Properties.SystemServiceResolver->BeginResolve(
                targetCuid_,
                this->ActivityHeader.ActivityId,
                this->RemainingTime,
                [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
                thisSPtr);
        }

        this->OnResolveCompleted(operation, true);
    }

    void EntreeService::ForwardToServiceOperation::OnResolveCompleted(
        AsyncOperationSPtr const & operation,
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        SystemServiceLocation primaryLocation;
        auto error = this->Properties.SystemServiceResolver->EndResolve(
            operation, 
            primaryLocation, 
            secondaryLocations_);

        if (error.IsSuccess())
        {
            auto instance = this->Properties.RequestInstance.GetNextInstance();

            this->ReceivedMessage->Headers.Replace(TimeoutHeader(this->RemainingTime));
            this->ReceivedMessage->Headers.Replace(primaryLocation.CreateFilterHeader());
            this->ReceivedMessage->Headers.Replace(GatewayRetryHeader(this->RetryCount));
            this->ReceivedMessage->Headers.Replace(RequestInstanceHeader(instance));

            auto const & node = primaryLocation.NodeInstance;
            
            this->RouteToNode(
                this->ReceivedMessage->Clone(),
                node.Id,
                node.InstanceId,
                true, // exact routing
                thisSPtr);
        }
        else if ((targetCuid_.IsFaultAnalysisService()) && (error.ReadValue() == ErrorCodeValue::FMFailoverUnitNotFound))
        {
            WriteInfo(
                TraceComponent,
                "{0}: retrying resolution of system service: name={1} cuid={2} error={3} - FAS is not present in the cluster",
                this->TraceId,
                targetServiceName_,
                targetCuid_,
                error);
            this->TryComplete(thisSPtr, error);
        }
        else if (NamingErrorCategories::IsRetryableAtGateway(error))
        {
            WriteInfo(
                TraceComponent, 
                "{0}: retrying resolution of system service: name={1} cuid={2} error={3}",
                this->TraceId,
                targetServiceName_,
                targetCuid_,
                error);

            this->HandleRetryStart(thisSPtr);
        }
        else
        {
            this->TryComplete(thisSPtr, error);
        }
    }

    bool EntreeService::ForwardToServiceOperation::TryValidateServiceTarget(
        Actor::Enum const & targetActor,
        ServiceTargetHeader const & targetHeader)
    {
        auto const & targetUri = targetHeader.ServiceName;

        switch (targetActor)
        {
        case Actor::IS:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicInfrastructureServiceName, 
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicInfrastructureServiceName is not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicInfrastructureServiceName);
            }

            return (expectedUriPrefix == targetUri) || expectedUriPrefix.IsPrefixOf(targetUri);
        }

        case Actor::NM:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicNamespaceManagerServiceName, 
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicNamespaceManagerServiceNameis not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicNamespaceManagerServiceName);
            }

            return expectedUriPrefix.IsPrefixOf(targetUri);
        }
        
        case Actor::FAS:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicFaultAnalysisServiceName,
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicFaultAnalysisServiceName is not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicFaultAnalysisServiceName);
            }

            return (expectedUriPrefix == targetUri) || expectedUriPrefix.IsPrefixOf(targetUri);
        }

        case Actor::UOS:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicUpgradeOrchestrationServiceName,
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicUpgradeOrchestrationServiceName is not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicUpgradeOrchestrationServiceName);
            }

            return (expectedUriPrefix == targetUri) || expectedUriPrefix.IsPrefixOf(targetUri);
        }

        case Actor::CSS:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicCentralSecretServiceName,
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicCentralSecretServiceName is not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicCentralSecretServiceName);
            }

            return (expectedUriPrefix == targetUri) || expectedUriPrefix.IsPrefixOf(targetUri);
        }

        case Actor::GatewayResourceManager:
        {
            NamingUri expectedUriPrefix;
            if (!NamingUri::TryParse(
                *SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName,
                expectedUriPrefix))
            {
                Assert::CodingError(
                    "SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName is not a valid NamingUri: {0}",
                    SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName);
            }

            return (expectedUriPrefix == targetUri) || expectedUriPrefix.IsPrefixOf(targetUri);
        }

        default:
            WriteWarning(
                TraceComponent,
                "{0} actor {1} does not support the given service target: {2}",
                this->TraceId,
                targetActor,
                targetUri);
            return false;
        }
    }

    bool EntreeService::ForwardToServiceOperation::TryInitializeStaticCuidTarget(Actor::Enum const & targetActor)
    {
        switch (targetActor)
        {
        case Actor::CM:
        case Actor::HM:
            //
            // CM and HM are currently co-located
            //
            targetCuid_ = ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::CMIdRange);
            break;

        case Actor::RM:
            targetCuid_ = ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::RMIdRange); 
            break;

        case Actor::FAS:
            targetCuid_ = ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::FaultAnalysisServiceIdRange);
            break;

        case Actor::UOS:
            targetCuid_ = ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::UpgradeOrchestrationServiceIdRange);
            break;

        case Actor::CSS:
            targetCuid_ = ConsistencyUnitId::CreateFirstReservedId(*ConsistencyUnitId::CentralSecretServiceIdRange);
            break;

        default:
            WriteWarning(TraceComponent, "{0} actor does not support static partition targets: {1}", this->TraceId, targetActor);
            return false;
        }

        return true;
    }

    void EntreeService::ForwardToServiceOperation::ProcessReply(Transport::MessageUPtr & reply)
    {
        // For the ListReply sent by FileStoreService, add the secondary locations.
        // This avoids the client from doing an extra resolve.
        //
        if (action_ == FileStoreServiceTcpMessage::ListAction || action_ == FileStoreServiceTcpMessage::GetStoreLocationsAction)
        {
            reply->Headers.Replace(SecondaryLocationsHeader(move(secondaryLocations_)));
        }        
    }

    void EntreeService::ForwardToServiceOperation::OnRouteToNodeFailedRetryableError(
        AsyncOperationSPtr const & thisSPtr,
        MessageUPtr & reply,
        ErrorCode const & error)
    {
        if (targetCuid_.IsInvalid)
        {
            this->Properties.SystemServiceResolver->MarkLocationStale(targetServiceName_);
        }
        else
        {
            this->Properties.SystemServiceResolver->MarkLocationStale(targetCuid_);
        }

        RequestAsyncOperationBase::OnRouteToNodeFailedRetryableError(thisSPtr, reply, error);
    }
}

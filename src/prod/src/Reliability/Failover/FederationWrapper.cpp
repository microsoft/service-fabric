// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace Reliability;
using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

StringLiteral const TraceSend("Send");
StringLiteral const TraceFMLocation("FMLocation");

NodeInstance MinInstance(NodeId::MinNodeId, 0);

FederationWrapper::FederationWrapper(Federation::FederationSubsystem & federation, ServiceResolver * serviceResolver)
    : RootedObject(federation.Root),
    federation_(federation),
    serviceResolver_(serviceResolver),
    traceId_(federation.IdString),
    fmLookupVersion_(0),
    fmGeneration_(),
    isFMPrimaryValid_(false),
    instanceIdStr_(federation_.Instance.ToString())
{
}

void FederationWrapper::SendToFM(MessageUPtr && message)
{
    NodeInstance fmInstance;
    if (!TryGetFMPrimary(fmInstance))
    {
        WriteInfo(TraceSend, traceId_,
            "Dropped message {0} id {1} since we do not have valid location of FM primary",
            message->Action, message->MessageId);
        return;
    }

    FailoverConfig const & config = FailoverConfig::GetConfig();

    message->Headers.Replace(TimeoutHeader(config.SendToFMTimeout));

    PartnerNodeSPtr node = federation_.GetNode(fmInstance);
    if (node)
    {
        if (node->Instance.InstanceId == fmInstance.InstanceId)
        {
            message->Idempotent = false;
            federation_.PToPSend(move(message), node);
        }
        else
        {
            InvalidateFMLocationIfNeeded(ErrorCodeValue::RoutingNodeDoesNotMatchFault, fmInstance);
        }

        return;
    }

    auto action = message->Action;
    federation_.BeginRoute(
        move(message),
        fmInstance.Id,
        fmInstance.InstanceId,
        true,
        TimeSpan::MaxValue,
        config.SendToFMTimeout,
        [this, action, fmInstance] (AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, fmInstance);
        },
        Root.CreateAsyncOperationRoot());
}

bool FederationWrapper::TryGetFMPrimary(Federation::NodeInstance & fmInstance)
{
    int64 fmLookupVersion;
    GenerationNumber fmGeneration;
    {
        AcquireReadLock grab(lock_);
        if (isFMPrimaryValid_)
        { 
            fmInstance = fmPrimaryNodeInstance_;
            return true; 
        }

        fmLookupVersion = fmLookupVersion_;
        fmGeneration = fmGeneration_;
    }

    auto inner = this->serviceResolver_->BeginResolveFMService(
        fmLookupVersion,
        fmGeneration,
        CacheMode::Refresh,
        FabricActivityHeader(),
        FailoverConfig::GetConfig().SendToFMTimeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnFMServiceResolved(operation, false);
        },
        Root.CreateAsyncOperationRoot());
    this->OnFMServiceResolved(inner, true);
    return false;
}

void FederationWrapper::UpdateFMService(ServiceTableEntry const & fmEntry, GenerationNumber const & generation)
{
    ServiceReplicaSet const & fmReplicaSet = fmEntry.ServiceReplicaSet;
    if (!fmReplicaSet.IsPrimaryLocationValid)
    {
        return;
    }

    NodeInstance fmPrimaryNodeInstance;
    bool result = NodeInstance::TryParse(fmReplicaSet.PrimaryLocation, fmPrimaryNodeInstance);
    ASSERT_IFNOT(result, "invalid FM replica set received: {0}", fmReplicaSet);

    AcquireWriteLock grab(lock_);
    if ((fmGeneration_ < generation) ||
        (fmGeneration_ == generation && fmLookupVersion_ < fmReplicaSet.LookupVersion))
    {
        fmLookupVersion_ = fmReplicaSet.LookupVersion;
        fmGeneration_ = generation;
        fmPrimaryNodeInstance_ = fmPrimaryNodeInstance;
        isFMPrimaryValid_= true;

        WriteInfo(TraceFMLocation, traceId_,
            "FM location updated to {0}, generation {1}, version {2}",
            fmPrimaryNodeInstance, fmGeneration_, fmLookupVersion_);
    }
    else
    {
        WriteInfo(TraceFMLocation, traceId_,
            "FM location update {0} generatioin {1} version {2} ignored, current generation {3} version {4}",
            fmPrimaryNodeInstance, generation, fmReplicaSet.LookupVersion, fmGeneration_, fmLookupVersion_);
    }
}

void FederationWrapper::OnFMServiceResolved(
    AsyncOperationSPtr const & operation,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != operation->CompletedSynchronously)
    {
        return;
    }

    ServiceTableEntry serviceTableEntry;
    GenerationNumber generation;
    ErrorCode error = this->serviceResolver_->EndResolveFMService(operation, serviceTableEntry, generation);
    if (!error.IsSuccess())
    {
        WriteWarning(TraceFMLocation, traceId_, "FM resolving failed with {0}", error);
        return;
    }

    // Ignore the result as it should have been pushed by FM change event
}

void FederationWrapper::SetFMMActor(Message & message)
{
    for (auto headerIter = message.Headers.Begin(); headerIter != message.Headers.End(); ++ headerIter)
    {
        if (headerIter->Id() == ActorHeader::Id)
        {
            message.Headers.Remove(headerIter);
            break;
        }
    }

    message.Headers.Add(Transport::ActorHeader(RSMessage::FMMActor));
}

void FederationWrapper::SendToFMM(Transport::MessageUPtr && message)
{
    SetFMMActor(*message);
    auto action = message->Action;

    FailoverConfig const & config = FailoverConfig::GetConfig();

    message->Headers.Replace(TimeoutHeader(config.SendToFMTimeout));

    federation_.BeginRoute(
        std::move(message),
        NodeId::MinNodeId,
        0,
        false,
        config.RoutingRetryTimeout,
        config.SendToFMTimeout,
        [this, action] (AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, MinInstance);
        },
        Root.CreateAsyncOperationRoot());
}

void FederationWrapper::SendToNode(MessageUPtr && message, NodeInstance const & target)
{
    PartnerNodeSPtr node = federation_.GetNode(target);
    if (node)
    {
        if (node->Instance.InstanceId == target.InstanceId)
        {
            message->Idempotent = false;
            federation_.PToPSend(move(message), node);
        }

        return;
    }

    auto action = message->Action;

    FailoverConfig const & config = FailoverConfig::GetConfig();

    message->Headers.Replace(TimeoutHeader(config.SendToNodeTimeout));

    federation_.BeginRoute(
        std::move(message),
        target.Id,
        target.InstanceId,
        true,
        TimeSpan::MaxValue,
        config.SendToNodeTimeout,
        [this, action, target] (AsyncOperationSPtr const& routeOperation)
        {
            this->RouteCallback(routeOperation, action, target);
        },
        Root.CreateAsyncOperationRoot());
}

void FederationWrapper::RouteCallback(
    AsyncOperationSPtr const& routeOperation,
    wstring const & action,
    NodeInstance const & target)
{
    ErrorCode error = federation_.EndRoute(routeOperation);
    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceSend,
            traceId_,
            "failed to send message {0} to node {1} with error {2}",
            action,
            target,
            error);
        InvalidateFMLocationIfNeeded(error, target);
    }
}

void FederationWrapper::InvalidateFMLocationIfNeeded(ErrorCode const & error, NodeInstance const & target)
{
    if ((error.ReadValue() != ErrorCodeValue::RoutingNodeDoesNotMatchFault) &&
            (error.ReadValue() != ErrorCodeValue::MessageHandlerDoesNotExistFault))
    {
        return;
    }

    int64 fmLookupVersion;
    GenerationNumber fmGeneration;
    {
        AcquireWriteLock grab(lock_);

        if ((target != fmPrimaryNodeInstance_) || !isFMPrimaryValid_) { return; }
        isFMPrimaryValid_ = false;
        fmLookupVersion = fmLookupVersion_;
        fmGeneration = fmGeneration_;
    }

    auto inner = this->serviceResolver_->BeginResolveFMService(
        fmLookupVersion,
        fmGeneration,
        CacheMode::Refresh,
        FabricActivityHeader(),
        FailoverConfig::GetConfig().SendToFMTimeout,
        [this](AsyncOperationSPtr const & operation)
        {
            this->OnFMServiceResolved(operation, false);
        },
        Root.CreateAsyncOperationRoot());
    this->OnFMServiceResolved(inner, true);
}

class FederationWrapper::FMRequestAsyncOperation : public AsyncOperation
{
    DENY_COPY(FMRequestAsyncOperation)

public:
    FMRequestAsyncOperation(
        FederationWrapper & federationWrapper,
        MessageUPtr && message,
        TimeSpan const retryInterval,
        TimeSpan const timeout, 
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & asyncOperationParent)
        : AsyncOperation(callback, asyncOperationParent),
        federationWrapper_(federationWrapper),
        message_(move(message)),
        retryInterval_(retryInterval),
        timeout_(timeout)
    {
    }

    static ErrorCode End(AsyncOperationSPtr const& operation, MessageUPtr & reply)
    {
        auto thisPtr = AsyncOperation::End<FMRequestAsyncOperation>(operation);
        reply = move(thisPtr->reply_);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (federationWrapper_.TryGetFMPrimary(fmInstance_))
        {
            federationWrapper_.BeginRequestToNode(
                move(message_),
                fmInstance_,
                true,
                retryInterval_,
                timeout_,
                [this] (AsyncOperationSPtr const & asyncOperation) { OnRequestCompleted(asyncOperation); },
                thisSPtr);
        }
        else
        {
            WriteInfo(TraceSend, federationWrapper_.traceId_,
                "Dropped message {0} id {1} since we do not have valid location of FM primary",
                message_->Action, message_->MessageId);

            this->TryComplete(thisSPtr, ErrorCodeValue::FMLocationNotKnown);
        }
    }

private:
    void OnRequestCompleted(AsyncOperationSPtr const & operation)
    {
        auto error = federationWrapper_.EndRequestToNode(operation, reply_);

        //
        // There's an error code (RoutingNodeDoesNotMatchFault) that the 
        // routing layer should ideally never surface since it's processed 
        // by the routing layer itself to indicate that the destination 
        // node is down. 
        //
        // However, there are many higher layer components that do need to process 
        // this error code (e.g. Multicast, Voters, FM, RA). So avoid surfacing 
        // routing layer errors directly to the caller when they only 
        // care about routing to FM (i.e. NS, CM).
        //
        // Issue: If node A routes to node B and B rejects with this error, then
        // that is okay. However, if A then further replies with this error to 
        // another node C (and C->A was also a routed request), then C will 
        // mistakenly think that A is down.
        //
        // FM query processing will return error codes directly rather than
        // wrapping them in a reply message body.
        //
        if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
        {
            error = ErrorCodeValue::FMLocationNotKnown;
        }

        this->TryComplete(operation->Parent, error); 

        federationWrapper_.InvalidateFMLocationIfNeeded(error, fmInstance_);
    }

    FederationWrapper & federationWrapper_;
    MessageUPtr message_;
    TimeSpan retryInterval_;
    TimeSpan timeout_;
    MessageUPtr reply_;
    NodeInstance fmInstance_;
};

AsyncOperationSPtr FederationWrapper::BeginRequestToFM(
    MessageUPtr && message,
    TimeSpan const retryInterval,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    // TODO: when not in reliable master mode, no need to create the context.
    return AsyncOperation::CreateAndStart<FMRequestAsyncOperation>(*this, move(message), retryInterval, timeout, callback, parent);
}

ErrorCode FederationWrapper::EndRequestToFM(AsyncOperationSPtr const & operation, MessageUPtr & reply)
{
    return FMRequestAsyncOperation::End(operation, reply);
}

AsyncOperationSPtr FederationWrapper::BeginRequestToFMM(
    MessageUPtr && message,
    TimeSpan const retryInterval,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    SetFMMActor(*message);
    return BeginRequestToNode(std::move(message), MinInstance, false, retryInterval, timeout, callback, parent);
}

ErrorCode FederationWrapper::EndRequestToFMM(AsyncOperationSPtr const & operation, MessageUPtr & reply) const
{
    return EndRequestToNode(operation, reply);
}

AsyncOperationSPtr FederationWrapper::BeginRequestToNode(
    MessageUPtr && message,
    NodeInstance const & target,
    bool useExactRouting,
    TimeSpan const retryInterval,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    message->Headers.Replace(TimeoutHeader(timeout));

    return federation_.BeginRouteRequest(
        std::move(message),
        target.Id,
        target.InstanceId,
        useExactRouting,
        retryInterval,
        timeout,
        callback,
        parent);
}

ErrorCode FederationWrapper::EndRequestToNode(AsyncOperationSPtr const & operation, MessageUPtr & reply) const
{
    return federation_.EndRouteRequest(operation, reply);
}

void FederationWrapper::CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Transport::MessageUPtr && reply)
{
    context.Reply(move(reply));
}

void FederationWrapper::CompleteRequestReceiverContext(Federation::RequestReceiverContext & context, Common::ErrorCode const & error)
{
    context.Reject(error);
}

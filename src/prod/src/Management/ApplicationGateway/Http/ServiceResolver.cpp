// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;
using namespace Client;
using namespace Naming;
using namespace HttpApplicationGateway;
using namespace Reliability;

class ServiceResolverImpl::OpenAsyncOperation : public TimedAsyncOperation
{
public:
    OpenAsyncOperation(
        ServiceResolverImpl &owner,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : TimedAsyncOperation(timeout, callback, parent)
        , owner_(owner)
    {}

    static ErrorCode End(AsyncOperationSPtr const &operation)
    {
        auto thisPtr = AsyncOperation::End<OpenAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        IClientFactoryPtr factoryPtr;
        auto error = ClientFactory::CreateLocalClientFactory(owner_.fabricNodeConfigSPtr_, factoryPtr);
        if (!error.IsSuccess())
        {
            HttpApplicationGatewayEventSource::Trace->CreateClientFactoryFail(
                error);

            TryComplete(thisSPtr, error);
            return;
        }

        error = factoryPtr->CreateServiceManagementClient(owner_.fabricClientPtr_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        //
        // Register for service endpoint change notifications for the root(fabric:/) - i.e. notifications
        // for all services in the cluster.
        //
        auto filter = make_shared<ServiceNotificationFilter>(
            NamingUri::RootNamingUri,
            ServiceNotificationFilterFlags::NamePrefix);

        auto operation = owner_.fabricClientPtr_->BeginRegisterServiceNotificationFilter(
            filter,
            RemainingTime,
            [this](AsyncOperationSPtr const &operation)
            {
                this->OnRegisterNotificationComplete(operation, false);
            },
            thisSPtr);

        OnRegisterNotificationComplete(operation, true);
    }

private:
    void OnRegisterNotificationComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        uint64 filterId;
        auto error = owner_.fabricClientPtr_->EndRegisterServiceNotificationFilter(operation, filterId);
        if (!error.IsSuccess())
        {
            HttpApplicationGatewayEventSource::Trace->NotificationRegistrationFail(
                error);
            TryComplete(operation->Parent, error);
            return;
        }

        TryComplete(operation->Parent, ErrorCodeValue::Success);
    }

    ServiceResolverImpl &owner_;
};

class ServiceResolverImpl::PrefixResolveAsyncOperation : public AllocableAsyncOperation
{
public:
    PrefixResolveAsyncOperation(
        ServiceResolverImpl &owner,
        ParsedGatewayUriSPtr const &parsedUri,
        IResolvedServicePartitionResultPtr &prevResult,
        wstring const& traceId,
        TimeSpan &timeout,
        AsyncCallback const &callback,
        AsyncOperationSPtr const &parent)
        : AllocableAsyncOperation(callback, parent)
        , owner_(owner)
        , parsedUri_(parsedUri)
        , prevResult_(prevResult)
        , traceId_(traceId)
        , timeoutHelper_(timeout)
    {}

    static ErrorCode End(AsyncOperationSPtr const &operation, IResolvedServicePartitionResultPtr &prevResult)
    {
        auto thisPtr = AsyncOperation::End<PrefixResolveAsyncOperation>(operation);

        if (thisPtr->Error.IsSuccess())
        {
            prevResult = thisPtr->prevResult_;
        }

        return thisPtr->Error;
    }

protected:

    void OnStart(AsyncOperationSPtr const &thisSPtr)
    {
        NamingUri uriPrefix;
        auto error = parsedUri_->TryGetServiceName(uriPrefix);
        if (!error.IsSuccess())
        {
            HttpApplicationGatewayEventSource::Trace->GetServiceNameFail(
                traceId_,
                parsedUri_->RequestUrlSuffix,
                error);

            // TODO: This error should be sent as errorcode value's message
            TryComplete(thisSPtr, ErrorCodeValue::InvalidAddress);
            return;
        }

        auto operation = owner_.fabricClientPtr_->BeginPrefixResolveServicePartitionInternal(
            uriPrefix,
            ServiceResolutionRequestData(parsedUri_->PartitionKey),
            prevResult_,
            false, // don't bypass cache
            Guid(traceId_),
            true,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const &operation)
            {
                OnResolveComplete(operation, false);
            },
            thisSPtr);

        OnResolveComplete(operation, true);
    }

private:

    void OnResolveComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ResolvedServicePartitionSPtr rsp;
        auto error = owner_.fabricClientPtr_->EndPrefixResolveServicePartitionInternal(operation, true, rsp);

        auto rspResult = make_shared<ResolvedServicePartitionResult>(rsp);
        prevResult_ = RootedObjectPointer<IResolvedServicePartitionResult>(
            rspResult.get(),
            rspResult->CreateComponentRoot());

        TryComplete(operation->Parent, error);
    }

    ServiceResolverImpl &owner_;
    ParsedGatewayUriSPtr parsedUri_;
    IResolvedServicePartitionResultPtr prevResult_;
    wstring traceId_;
    TimeoutHelper timeoutHelper_;
};

AsyncOperationSPtr ServiceResolverImpl::BeginOpen(
    TimeSpan &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
}

ErrorCode ServiceResolverImpl::EndOpen(AsyncOperationSPtr const &operation)
{
    return OpenAsyncOperation::End(operation);
}

AsyncOperationSPtr ServiceResolverImpl::BeginPrefixResolveServicePartition(
    ParsedGatewayUriSPtr const &parsedUri,
    IResolvedServicePartitionResultPtr &prevResult,
    wstring const& traceId,
    KAllocator &allocator,
    TimeSpan &timeout,
    AsyncCallback const &callback,
    AsyncOperationSPtr const &parent)
{
    return AllocableAsyncOperation::CreateAndStart<PrefixResolveAsyncOperation>(
        allocator,
        *this,
        parsedUri,
        prevResult,
        traceId,
        timeout,
        callback,
        parent);
}

ErrorCode ServiceResolverImpl::EndResolveServicePartition(
    AsyncOperationSPtr const &operation,
    IResolvedServicePartitionResultPtr &prevResult)
{
    return PrefixResolveAsyncOperation::End(operation, prevResult);
}

ErrorCode ServiceResolverImpl::GetReplicaEndpoint(
    IResolvedServicePartitionResultPtr const &rspResult,
    TargetReplicaSelector::Enum replicaSelector,
    __out wstring &replicaLocation)
{
    //
    // TODO: This can be made common code.
    //
    auto replicaSet = rspResult->GetServiceReplicaSet();

    if (replicaSet.IsEmpty())
    {
        Assert::TestAssert("RSP.GetEndpoint: there should be at least one endpoint");

        HttpApplicationGatewayEventSource::Trace->GetServiceReplicaSetFail(
            GetServiceName(rspResult));

        return ErrorCodeValue::ServiceOffline;
    }

    if (replicaSet.IsStateful)
    {
        if (replicaSelector == TargetReplicaSelector::PrimaryReplica)
        {
            if (!replicaSet.IsPrimaryLocationValid)
            {
                return ErrorCodeValue::ServiceOffline;
            }
            replicaLocation = replicaSet.PrimaryLocation;
        }
        else if (replicaSelector == TargetReplicaSelector::RandomSecondaryReplica)
        {
            size_t instances = replicaSet.SecondaryLocations.size();
            if (instances == 0)
            {
                return ErrorCodeValue::NotFound;
            }

            Random r;
            size_t endpointIndex = static_cast<size_t>(r.Next(static_cast<int>(instances)));
            replicaLocation = replicaSet.SecondaryLocations[endpointIndex];
        }
        else // random replica = primary or secondary
        {
            size_t instances = replicaSet.SecondaryLocations.size() + (replicaSet.IsPrimaryLocationValid ? 1 : 0);
            Random r;
            size_t endpointIndex = static_cast<size_t>(r.Next(static_cast<int>(instances)));
            if ((endpointIndex == (instances - 1)) && replicaSet.IsPrimaryLocationValid)
            {
                replicaLocation = replicaSet.PrimaryLocation;
            }
            else
            {
                replicaLocation = replicaSet.SecondaryLocations[endpointIndex];
            }
        }
    }
    else
    {
        if (replicaSelector != TargetReplicaSelector::RandomInstance)
        {
            return ErrorCodeValue::InvalidArgument;
        }

        size_t instances = replicaSet.ReplicaLocations.size();
        Random r;
        size_t statelessEndpointIndex = static_cast<size_t>(r.Next(static_cast<int>(instances)));
        replicaLocation = replicaSet.ReplicaLocations[statelessEndpointIndex];
    }

    return ErrorCodeValue::Success;
}

wstring ServiceResolverImpl::GetServiceName(IResolvedServicePartitionResultPtr const &result)
{
    ResolvedServicePartitionResult *rspResult = (ResolvedServicePartitionResult *)result.get();
    return rspResult->ResolvedServicePartition->Locations.ServiceName;
}

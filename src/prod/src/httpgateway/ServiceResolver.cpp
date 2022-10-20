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
using namespace HttpGateway;

StringLiteral const TraceType("HttpGatewayServiceResolver");
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
            WriteWarning(
                TraceType,
                "Creation of fabricclient factory failed: error = {0}",
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

        TryComplete(thisSPtr, ErrorCodeValue::Success);
    }

    ServiceResolverImpl &owner_;
};

class ServiceResolverImpl::PrefixResolveAsyncOperation : public AllocableAsyncOperation
{
public:
    PrefixResolveAsyncOperation(
        ServiceResolverImpl &owner,
        std::shared_ptr<HttpApplicationGateway::ParsedGatewayUri> const &parsedUri,
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
            WriteWarning(
                TraceType,
                "{0} The ServiceName from URI cannot be parsed as Fabric URI : URI = {1}, error = {2}.",
                traceId_,
                parsedUri_->RequestUrlSuffix,
                error);

            TryComplete(thisSPtr, ErrorCodeValue::InvalidAddress);
            return;
        }

        NamingUri systemUriPrefix;
        NamingUri::TryParse(L"fabric:/System", systemUriPrefix);
        if (systemUriPrefix.IsPrefixOf(uriPrefix))
        {
            // fabric:/System URI.
            // Get internal System service name and resolve
            auto internalServiceName = ServiceModel::SystemServiceApplicationNameHelper::GetInternalServiceName(uriPrefix.ToString());
            NamingUri::TryParse(internalServiceName, uriPrefix);

            auto operation = owner_.fabricClientPtr_->BeginResolveSystemServicePartition(
                uriPrefix,
                parsedUri_->PartitionKey,
                Guid(traceId_),
                timeoutHelper_.GetRemainingTime(),
                [this](AsyncOperationSPtr const &operation)
            {
                OnResolveSystemServiceComplete(operation, false);
            },
                thisSPtr);

            OnResolveSystemServiceComplete(operation, true);
        }
        else
        {
            // Not a system service, fail the request
            WriteWarning(
                TraceType,
                "{0} ServiceName from URI {1} does not belong to a system service, failing the request.",
                traceId_,
                parsedUri_->RequestUrlSuffix);

            TryComplete(thisSPtr, ErrorCodeValue::InvalidAddress);
            return;
        }
    }

private:

    void OnResolveSystemServiceComplete(AsyncOperationSPtr const &operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously)
        {
            return;
        }

        ResolvedServicePartitionSPtr rsp;
        auto error = owner_.fabricClientPtr_->EndResolveSystemServicePartition(operation, rsp);

        auto rspResult = make_shared<ResolvedServicePartitionResult>(rsp);
        prevResult_ = RootedObjectPointer<IResolvedServicePartitionResult>(
            rspResult.get(),
            rspResult->CreateComponentRoot());

        TryComplete(operation->Parent, error);
    }

    ServiceResolverImpl &owner_;
    std::shared_ptr<HttpApplicationGateway::ParsedGatewayUri> parsedUri_;
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
    std::shared_ptr<HttpApplicationGateway::ParsedGatewayUri> const &parsedUri,
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
    HttpApplicationGateway::TargetReplicaSelector::Enum replicaSelector,
    __out wstring &replicaLocation)
{
    //
    // TODO: This can be made common code.
    //
    auto replicaSet = rspResult->GetServiceReplicaSet();

    if (replicaSet.IsEmpty())
    {
        Assert::TestAssert("RSP.GetEndpoint: there should be at least one endpoint");
        WriteWarning(
            TraceType,
            "RSP for service returned with 0 endpoints: service name = {1}.",
            GetServiceName(rspResult));

        return ErrorCodeValue::ServiceOffline;
    }

    if (replicaSet.IsStateful)
    {
        if (replicaSelector == HttpApplicationGateway::TargetReplicaSelector::PrimaryReplica)
        {
            if (replicaSet.IsPrimaryLocationValid)
            {
                replicaLocation = replicaSet.PrimaryLocation;
            }
            else
            {
                return ErrorCodeValue::ServiceOffline;
            }
        }
        else if (replicaSelector == HttpApplicationGateway::TargetReplicaSelector::RandomSecondaryReplica)
        {
            size_t instances = replicaSet.SecondaryLocations.size();
            Random r;
            size_t endpointIndex = static_cast<size_t>(r.Next(static_cast<int>(instances)));
            replicaLocation = replicaSet.SecondaryLocations[endpointIndex];
        }
        else // random replica = primary or secondary
        {
            size_t instances = replicaSet.SecondaryLocations.size() + 1;
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
        if (replicaSelector != HttpApplicationGateway::TargetReplicaSelector::RandomInstance)
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

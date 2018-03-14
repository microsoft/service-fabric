// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Transport;
using namespace Reliability;
using namespace Query;
using namespace Naming;

using namespace SystemServices;

// *** Private ServiceDirectMessagingClient::Impl

StringLiteral const TraceComponent("ServiceDirectMessagingClient");

class ServiceDirectMessagingClient::Impl 
    : public RootedObject
{
public:
    Impl(
        __in IDatagramTransport & transport,
        __in SystemServiceResolver & resolver,
        ComponentRoot const & root)
        : RootedObject(root)
        , transport_(transport)
        , resolver_(resolver)
        , transportTargetsByName_()
        , cuidsLock_()
    {
    }

    AsyncOperationSPtr BeginResolve(
        wstring const & serviceName,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root);

    ErrorCode EndResolve(
        AsyncOperationSPtr const & operation, 
        __out SystemServiceLocation & primaryLocation,
        __out ISendTarget::SPtr & primaryTarget);

    ISendTarget::SPtr UpdateAndGetTransportTarget(wstring const & svcName, wstring const & hostAddress)
    {
        bool update = false;
        {
            AcquireReadLock lock(cuidsLock_);

            auto it = transportTargetsByName_.find(svcName);

            if (it == transportTargetsByName_.end() || it->second->Address() != hostAddress)
            {
                update = true;
            }
            else
            {
                return it->second;
            }
        }

        if (update)
        {
            AcquireWriteLock lock(cuidsLock_);

            auto it = transportTargetsByName_.find(svcName);

            if (it == transportTargetsByName_.end() || it->second->Address() != hostAddress)
            {
                auto target = transport_.ResolveTarget(hostAddress);

                if (target)
                {
                    transportTargetsByName_[svcName] = target;

                    return target;
                }
            }
        }

        return ISendTarget::SPtr();
    }

    void ClearTransportTarget(wstring const & svcName)
    {
        AcquireWriteLock lock(cuidsLock_);

        transportTargetsByName_.erase(svcName);
    }

private:
    class ResolveAsyncOperation;

    IDatagramTransport & transport_;
    SystemServiceResolver & resolver_;

    unordered_map<wstring, ISendTarget::SPtr> transportTargetsByName_;
    RwLock cuidsLock_;
};

// ***  ResolveAsyncOperation

class ServiceDirectMessagingClient::Impl::ResolveAsyncOperation 
    : public AsyncOperation
    , public TextTraceComponent<TraceTaskCodes::SystemServices>
{
public:
    ResolveAsyncOperation(
        __in ServiceDirectMessagingClient::Impl & owner,
        wstring const & serviceName,
        ActivityId const & activityId,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
        : AsyncOperation(callback, root)
        , owner_(owner)
        , serviceName_(serviceName)
        , activityId_(activityId)
        , traceId_(wformatString("{0}", activityId_))
        , timeoutHelper_(timeout)
        , primaryLocation_()
        , primaryTarget_()
    {
    }

    static ErrorCode End(
        AsyncOperationSPtr const & operation, 
        __out SystemServiceLocation & primaryLocation,
        __out ISendTarget::SPtr & primaryTarget)
    {
        auto casted = AsyncOperation::End<ResolveAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            primaryLocation = casted->primaryLocation_;
            primaryTarget = casted->primaryTarget_;
        }

        return casted->Error;
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto operation = owner_.resolver_.BeginResolve(
            serviceName_,
            activityId_,
            timeoutHelper_.GetRemainingTime(),
            [this](AsyncOperationSPtr const & operation) { this->OnResolveCompleted(operation, false); },
            thisSPtr);
        this->OnResolveCompleted(operation, true);
    }

private:

    __declspec (property(get=get_TraceId)) wstring const & TraceId;
    wstring const & get_TraceId() const { return traceId_; }

    void OnResolveCompleted(AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto const & thisSPtr = operation->Parent;

        vector<wstring> secondaryLocations;
        auto error = owner_.resolver_.EndResolve(operation, primaryLocation_, secondaryLocations);

        if (error.IsSuccess())
        {
            primaryTarget_ = owner_.UpdateAndGetTransportTarget(serviceName_, primaryLocation_.HostAddress);
            
            if (!primaryTarget_)
            {
                WriteInfo(
                    TraceComponent, 
                    "{0} failed to resolve send target for address {1}", 
                    this->TraceId, 
                    primaryLocation_.HostAddress);

                error = ErrorCodeValue::SystemServiceNotFound;
            }
        }
        else if (error.IsError(ErrorCodeValue::FMFailoverUnitNotFound))
        {
            owner_.ClearTransportTarget(serviceName_);
        }

        this->TryComplete(thisSPtr, error);
    }

    ServiceDirectMessagingClient::Impl & owner_;
    wstring serviceName_;
    ActivityId activityId_;

    wstring traceId_;
    TimeoutHelper timeoutHelper_;

    SystemServiceLocation primaryLocation_;
    ISendTarget::SPtr primaryTarget_;
};

// *** Impl

AsyncOperationSPtr ServiceDirectMessagingClient::Impl::BeginResolve(
    wstring const & serviceName,
    ActivityId const & activityId,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & root)
{
    return AsyncOperation::CreateAndStart<ResolveAsyncOperation>(
        *this,
        serviceName,
        activityId,
        timeout,
        callback,
        root);
}

ErrorCode ServiceDirectMessagingClient::Impl::EndResolve(
    AsyncOperationSPtr const & operation, 
    __out SystemServiceLocation & primaryLocation,
    __out ISendTarget::SPtr & primaryTarget)
{
    return ResolveAsyncOperation::End(operation, primaryLocation, primaryTarget);
}

// *** Public ServiceDirectMessagingClient

ServiceDirectMessagingClientSPtr ServiceDirectMessagingClient::Create(
    __in Transport::IDatagramTransport & transport,
    __in SystemServiceResolver & resolver,
    Common::ComponentRoot const & root)
{
    return shared_ptr<ServiceDirectMessagingClient>(new ServiceDirectMessagingClient(transport, resolver, root));
}

ServiceDirectMessagingClient::ServiceDirectMessagingClient(
    __in Transport::IDatagramTransport & transport,
    __in SystemServiceResolver & resolver,
    Common::ComponentRoot const & root)
    : implUPtr_(make_unique<Impl>(transport, resolver, root))
{
}

Common::AsyncOperationSPtr ServiceDirectMessagingClient::BeginResolve(
    std::wstring const & serviceName,
    Common::ActivityId const & activityId,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & root)
{
    return implUPtr_->BeginResolve(serviceName, activityId, timeout, callback, root);
}

Common::ErrorCode ServiceDirectMessagingClient::EndResolve(
    Common::AsyncOperationSPtr const & operation, 
    __out SystemServiceLocation & primaryLocation,
    __out ISendTarget::SPtr & primaryTarget)
{
    return implUPtr_->EndResolve(operation, primaryLocation, primaryTarget);
}

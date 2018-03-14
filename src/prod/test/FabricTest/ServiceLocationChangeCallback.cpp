// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Naming;

StringLiteral const TraceSource("ServiceLocationChangeCallback");

extern HRESULT TryConvertComResult(
    ComPointer<IFabricResolvedServicePartitionResult> const & comResult, 
    __out ResolvedServicePartition & cResult);

ServiceLocationChangeCallbackCPtr ServiceLocationChangeCallback::Create(
    Common::ComPointer<IFabricServiceManagementClient> const & client,
    std::wstring const & uri,
    FABRIC_PARTITION_KEY_TYPE keyType,
    void const * key)
{
    switch (keyType)
    {
        case FABRIC_PARTITION_KEY_TYPE_INT64:
            return make_com<ServiceLocationChangeCallback>(client, uri, *reinterpret_cast<__int64 const *>(key));
        case FABRIC_PARTITION_KEY_TYPE_STRING:
            return make_com<ServiceLocationChangeCallback>(client, uri, wstring(reinterpret_cast<LPCWSTR>(key)));
        default: 
            return make_com<ServiceLocationChangeCallback>(client, uri);
    }
}

ServiceLocationChangeCallback::ServiceLocationChangeCallback(
    Common::ComPointer<IFabricServiceManagementClient> const & client,
    std::wstring const & uri)
    : client_(client)
    , uri_(uri)
    , keyType_(FABRIC_PARTITION_KEY_TYPE_NONE)
    , intKey_(-1)
    , stringKey_()
    , callbackHandler_(0)
    , changes_()
    , callbackCalledEvent_(false)
    , lock_()
{
}

ServiceLocationChangeCallback::ServiceLocationChangeCallback(
    Common::ComPointer<IFabricServiceManagementClient> const & client,
    std::wstring const & uri,
    __int64 key)
    : client_(client)
    , uri_(uri)
    , keyType_(FABRIC_PARTITION_KEY_TYPE_INT64)
    , intKey_(key)
    , stringKey_()
    , callbackHandler_(0)
    , changes_()
    , callbackCalledEvent_(false)
    , lock_()
{
}

ServiceLocationChangeCallback::ServiceLocationChangeCallback(
    Common::ComPointer<IFabricServiceManagementClient> const & client,
    std::wstring const & uri,
    wstring const & key)
    : client_(client)
    , uri_(uri)
    , keyType_(FABRIC_PARTITION_KEY_TYPE_STRING)
    , intKey_(-1)
    , stringKey_(key)
    , callbackHandler_(0)
    , changes_()
    , callbackCalledEvent_(false)
    , lock_()
{
}
    
HRESULT ServiceLocationChangeCallback::Register()
{
    void const * key = NULL;
    switch (keyType_)
    {
        case FABRIC_PARTITION_KEY_TYPE_INT64:
            key = static_cast<void const *>(&intKey_);
            break;
        case FABRIC_PARTITION_KEY_TYPE_STRING:
           key = static_cast<void const *>(stringKey_.c_str());
           break;
        default: break;
    }

    HRESULT hr;
    LONGLONG handlerId;
    hr = client_->RegisterServicePartitionResolutionChangeHandler(
        uri_.c_str(),
        keyType_,
        key,
        this,
        &handlerId);

    {
        AcquireExclusiveLock lock(lock_);
        UpdateHandlerIdCallerHoldsLock(handlerId);
    }

    return hr;
}

HRESULT ServiceLocationChangeCallback::Unregister()
{
    LONGLONG handlerId;

    {
        AcquireExclusiveLock lock(lock_);
        TestSession::FailTestIf(0 == callbackHandler_, "0 callbackHandler_");
        handlerId = callbackHandler_;
    }

    return client_->UnregisterServicePartitionResolutionChangeHandler(handlerId);
}

bool ServiceLocationChangeCallback::WaitForCallback(Common::TimeSpan const timeout, HRESULT expectedError)
{
    // Check whether there are changes not yet looked at
    if (TryMatchChange(expectedError))
    {
        return true;
    }

    TimeoutHelper timeoutHelper(timeout);
    do
    {
        // Wait for a new callback to be triggered
        if (!callbackCalledEvent_.WaitOne(timeoutHelper.GetRemainingTime()))
        {
            break;
        }

        // Callback was called, check the errors again
        if (TryMatchChange(expectedError))
        {
            return true;
        }
    }
    while(!timeoutHelper.IsExpired);

    // Timeout reached, return success if timeout was expected
    return expectedError == ErrorCode(ErrorCodeValue::Timeout).ToHResult();
}

void STDMETHODCALLTYPE ServiceLocationChangeCallback::OnChange(
    IFabricServiceManagementClient * source,
    LONGLONG handlerId,
    IFabricResolvedServicePartitionResult * partition,
    HRESULT error)
{
    TestSession::FailTestIf(source != client_.GetRawPointer(), "OnChange: IFabricServiceManagementClient does not match");
    
    AddChange(handlerId, partition, error);
    
    callbackCalledEvent_.Set();
}

void ServiceLocationChangeCallback::UpdateHandlerIdCallerHoldsLock(LONGLONG handlerId)
{
    if (callbackHandler_ == 0)
    {
        callbackHandler_ = handlerId;
    }
    else
    {
        TestSession::FailTestIf(handlerId != callbackHandler_, "AddChange: HandlerId does not match");
    }
}

void ServiceLocationChangeCallback::AddChange(
    LONGLONG handlerId,
    IFabricResolvedServicePartitionResult * partition, 
    HRESULT hr)
{
    AcquireExclusiveLock lock(lock_);

    UpdateHandlerIdCallerHoldsLock(handlerId);
    
    Common::ComPointer<IFabricResolvedServicePartitionResult> newPartition;
    if (partition != NULL)
    {
        if (!changes_.empty())
        {
            ComPointer<IFabricResolvedServicePartitionResult> const & previous = changes_[changes_.size() - 1].first;
            if (previous)
            {
                LONG compareResult;
                HRESULT hr2 = previous->CompareVersion(partition, &compareResult);
                TestSession::FailTestIf(FAILED(hr2), "Callback {0} CompareVersion failed. hresult = {1}", callbackHandler_, hr2);    
                TestSession::FailTestIf(compareResult >= 0, "Callback {0} CompareVersion: new partition is not higher than old partition", callbackHandler_);
            }
        }

        newPartition.SetAndAddRef(partition);
        ResolvedServicePartition resolvedServiceLocations;
        HRESULT hr3 = TryConvertComResult(newPartition, resolvedServiceLocations);
        TestSession::FailTestIf(FAILED(hr3), "Callback {0} Converting COM service results failed. hresult = {1}", callbackHandler_, hr3);

        TestSession::WriteInfo(TraceSource, "Callback {0} called with rsp {1}", callbackHandler_, resolvedServiceLocations);
    }

    ServicePartitionResolutionChange change(move(newPartition), hr);
    changes_.push_back(move(change));
}

bool ServiceLocationChangeCallback::TryMatchChange(HRESULT expectedError)
{
    AcquireExclusiveLock lock(lock_);
    if (!changes_.empty())
    {
        auto firstMatchIter = find_if(changes_.begin(), changes_.end(), 
            [expectedError] (ServicePartitionResolutionChange const & change) -> bool { return change.second == expectedError; });
        if (firstMatchIter != changes_.end())
        {
            // Clear all entries triggered before the matched value
            TestSession::WriteInfo(TraceSource, "Callback {0} got called with expected failure {1}.", callbackHandler_, expectedError);
            changes_.erase(changes_.begin(), firstMatchIter);
            return true;
        }
             
        // Clear all entries
        changes_.clear();
    }

    return false;
}

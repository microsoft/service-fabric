// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace FabricTest;
using namespace TestCommon;

HRESULT ServiceLocationChangeClient::AddCallback(
    std::wstring const & callbackName, 
    std::wstring const & uri, 
    FABRIC_PARTITION_KEY_TYPE keyType, 
    void const * key)
{
    if (callbacks_.end() == callbacks_.find(callbackName))
    {
        ServiceLocationChangeCallbackCPtr callback = ServiceLocationChangeCallback::Create(client_, uri, keyType, key);
        HRESULT hr = callback->Register();
        if (FAILED(hr))
        {
            TestSession::WriteError(TraceSource, "AddCallback with name: {0} returned {1}.", callbackName, hr);
        }
        else
        {
            callbacks_.insert(std::make_pair(callbackName, callback));
        }

        return hr;
    }
    else
    {
        TestSession::WriteError(TraceSource, "Callback with name: {0} already exists.", callbackName);
        return ErrorCode(ErrorCodeValue::AlreadyExists).ToHResult();
    }
}

bool ServiceLocationChangeClient::DeleteCallback(std::wstring const & callbackName)
{
    std::map<std::wstring, ServiceLocationChangeCallbackCPtr>::iterator iteratorCallback = callbacks_.find(callbackName);
    if (callbacks_.end() != iteratorCallback)
    {
        iteratorCallback->second->Unregister();
        callbacks_.erase(iteratorCallback);
        return true;
    }
    else
    {
        TestSession::WriteError(TraceSource, "Callback with name: {0} does not exists.", callbackName);
        return false;
    }
}

bool ServiceLocationChangeClient::WaitForCallback(std::wstring const & callbackName, TimeSpan const & timeout, HRESULT expectedError)
{
    std::map<std::wstring, ServiceLocationChangeCallbackCPtr>::iterator iteratorCallback = callbacks_.find(callbackName);
    if (callbacks_.end() != iteratorCallback)
    {
        // Wait until the desired hresult is encountered or timeout is reached.
        return iteratorCallback->second->WaitForCallback(timeout, expectedError);
    }
    else
    {
        TestSession::WriteError(TraceSource, "Callback with name: {0} does not exists.", callbackName);
        return false;
    }
}

bool ServiceLocationChangeClient::GetCacheInfo(
    std::wstring const & cacheItemName,
    __out ServiceLocationChangeCacheInfoSPtr& cache) const
{
    AcquireReadLock grab(lock_);
    auto iteratorCacheItem = cacheItems_.find(cacheItemName);
    if (cacheItems_.end() != iteratorCacheItem)
    {
        cache = iteratorCacheItem->second;
        return true;
    }
    return false;
}

bool ServiceLocationChangeClient::AddCacheItem(
    std::wstring const & cacheItemName,
    std::wstring const & uri,
    FABRIC_PARTITION_KEY_TYPE keyType,
    void const * key,
    __in Common::ComPointer<IFabricResolvedServicePartitionResult> & cacheItem,
    bool checkCompareVersion,
    HRESULT expectedCompareVersionError)
{
    AcquireWriteLock grab(lock_);
    auto iteratorCacheItem = cacheItems_.find(cacheItemName);
    if (cacheItems_.end() != iteratorCacheItem)
    {
        auto cached = iteratorCacheItem->second;
        TestSession::WriteNoise(TraceSource, "Cache item with name {0} already exists, replacing.", cacheItemName);

        if (checkCompareVersion)
        {
            LONG compareResult;
            auto hr = cached->CacheItem->CompareVersion(cacheItem.GetRawPointer(), &compareResult);
            
            // The new cache item must be higher or equal to the previous value
            if (SUCCEEDED(hr) && (compareResult > 0) && (cached->ServiceName == uri))
            {
                TestSession::FailTest("Service {0}, name {1}: Can't add resolved service partition with lower version than the currently cached one", uri, cacheItemName);
            }

            TestSession::FailTestIfNot(
                hr == expectedCompareVersionError, 
                "AddCacheItem->cacheInfo.CompareVersion: expected {0}, actual {1}",
                expectedCompareVersionError,
                hr);
        }
    }
    else
    {
        TestSession::WriteNoise(TraceSource, "Add service resolution for service {0}, name {1}", uri, cacheItemName);
    }

    ServiceLocationChangeCacheInfoSPtr cacheInfo;
    if (keyType == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_INT64)
    {
        int64 keyInt64 = *reinterpret_cast<__int64 const *>(key);
        cacheInfo = std::make_shared<ServiceLocationChangeCacheInfo>(uri, keyInt64, cacheItem);
    }
    else if (keyType == FABRIC_PARTITION_KEY_TYPE::FABRIC_PARTITION_KEY_TYPE_STRING)
    {
        wstring keyString(reinterpret_cast<LPCWSTR>(key));
        cacheInfo = std::make_shared<ServiceLocationChangeCacheInfo>(uri, keyString, cacheItem);
    }
    else
    {
        cacheInfo = std::make_shared<ServiceLocationChangeCacheInfo>(uri, cacheItem);
    }

    cacheItems_[cacheItemName] = move(cacheInfo);

    return true;
}

bool ServiceLocationChangeClient::DeleteCacheItem(std::wstring const & cacheItemName)
{
    std::map<std::wstring, ServiceLocationChangeCacheInfoSPtr>::iterator iteratorCacheItem = cacheItems_.find(cacheItemName);
    if (cacheItems_.end() == iteratorCacheItem)
    {
        TestSession::WriteError(TraceSource, "Cache item with name: {0} does not exists.", cacheItemName);
        return false;
    }
    else
    {
        cacheItems_.erase(iteratorCacheItem);
        return true;
    }
}

const Common::StringLiteral ServiceLocationChangeClient::TraceSource("FabricTest.ServiceLocationChangeClient");

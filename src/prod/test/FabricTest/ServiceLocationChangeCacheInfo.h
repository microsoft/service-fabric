// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServiceLocationChangeCacheInfo
    {
        DENY_COPY(ServiceLocationChangeCacheInfo);
        
    public:
        ServiceLocationChangeCacheInfo(
            std::wstring const & serviceName,
            __int64 key,
            Common::ComPointer<IFabricResolvedServicePartitionResult>& cacheItem)
            : serviceName_(serviceName)
            , keyType_(FABRIC_PARTITION_KEY_TYPE_INT64)
            , intKey_(key)
            , stringKey_()
            , cacheItem_(cacheItem)
        {
        }

        ServiceLocationChangeCacheInfo(
            std::wstring const & serviceName,
            std::wstring const & key,
            Common::ComPointer<IFabricResolvedServicePartitionResult>& cacheItem)
            : serviceName_(serviceName)
            , keyType_(FABRIC_PARTITION_KEY_TYPE_STRING)
            , intKey_(-1)
            , stringKey_(key)
            , cacheItem_(cacheItem)
        {
        }

        ServiceLocationChangeCacheInfo(
            std::wstring const & serviceName,
            Common::ComPointer<IFabricResolvedServicePartitionResult>& cacheItem)
            : serviceName_(serviceName)
            , keyType_(FABRIC_PARTITION_KEY_TYPE_NONE)
            , intKey_(-1)
            , stringKey_()
            , cacheItem_(cacheItem)
        {
        }
        
        __declspec (property(get=get_ServiceName)) std::wstring ServiceName;
        std::wstring get_ServiceName() { return serviceName_; }
        
        __declspec (property(get=get_CacheItem)) Common::ComPointer<IFabricResolvedServicePartitionResult> CacheItem;
        Common::ComPointer<IFabricResolvedServicePartitionResult> get_CacheItem() { return cacheItem_; }
        
    private:
        std::wstring serviceName_;
        FABRIC_PARTITION_KEY_TYPE keyType_;
        __int64 intKey_;
        std::wstring stringKey_;
        Common::ComPointer<IFabricResolvedServicePartitionResult> cacheItem_;
    };
    typedef std::shared_ptr<ServiceLocationChangeCacheInfo> ServiceLocationChangeCacheInfoSPtr;
}

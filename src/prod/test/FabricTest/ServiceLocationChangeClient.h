// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServiceLocationChangeClient
    {
        DENY_COPY(ServiceLocationChangeClient);
        
    public:
        ServiceLocationChangeClient(
            Common::ComPointer<IFabricServiceManagementClient> const & client) 
            : client_(client)
            , callbacks_()
            , cacheItems_()
            , lock_()
        {
        }
       
        typedef std::map<std::wstring, ServiceLocationChangeCacheInfoSPtr> ServiceLocationChangeCacheInfoMap;

        __declspec (property(get=get_CacheItems)) ServiceLocationChangeCacheInfoMap const & CacheItems;
        ServiceLocationChangeCacheInfoMap const & get_CacheItems() const { return cacheItems_; }

        __declspec (property(get=get_Client)) Common::ComPointer<IFabricServiceManagementClient> Client;
        Common::ComPointer<IFabricServiceManagementClient> get_Client() { return client_; }

        HRESULT AddCallback(
            std::wstring const & callbackName, 
            std::wstring const & uri, 
            FABRIC_PARTITION_KEY_TYPE keyType, 
            void const * key);

        bool WaitForCallback(std::wstring const & callbackName, Common::TimeSpan const & timeout, HRESULT expectedError);
            
        bool DeleteCallback(std::wstring const & callbackName);

        bool GetCacheInfo(
            std::wstring const & cacheItemName, 
            __out ServiceLocationChangeCacheInfoSPtr& cache) const;
        
        bool AddCacheItem(
            std::wstring const & cacheItemName,
            std::wstring const & uri,
            FABRIC_PARTITION_KEY_TYPE keyType,
            void const * key,
            __in Common::ComPointer<IFabricResolvedServicePartitionResult> & cacheItem,
            bool checkCompareVersion,
            HRESULT expectedCompareVersionError);

        bool DeleteCacheItem(std::wstring const & cacheItemName);

    private:
        Common::ComPointer<IFabricServiceManagementClient> client_;
        std::map<std::wstring, ServiceLocationChangeCallbackCPtr> callbacks_; // key on callback name
        ServiceLocationChangeCacheInfoMap cacheItems_;
        mutable Common::RwLock lock_;

        static const Common::StringLiteral TraceSource;
    };

    typedef std::shared_ptr<ServiceLocationChangeClient> ServiceLocationChangeClientSPtr;
}

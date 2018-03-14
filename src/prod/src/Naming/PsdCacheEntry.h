// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PsdCacheEntryBase : public Common::LruCacheEntryBase<std::wstring>
    {
    public:
        explicit PsdCacheEntryBase(PartitionedServiceDescriptor && psd) 
            : LruCacheEntryBase(psd.Service.Name)
            , psd_(std::make_shared<PartitionedServiceDescriptor>(std::move(psd))) 
        { 
        }

        explicit PsdCacheEntryBase(std::wstring const & name)
            : LruCacheEntryBase(name)
            , psd_()
        { 
        }

        __declspec(property(get=get_HasPsd)) bool HasPsd;

        bool get_HasPsd() const { return (psd_ != nullptr); }

        __declspec(property(get=get_Psd)) std::shared_ptr<PartitionedServiceDescriptor> const & Psd;
        std::shared_ptr<PartitionedServiceDescriptor> const & get_Psd() const { return psd_; }

        static size_t GetHash(std::wstring const & name)
        { 
            return Common::StringUtility::GetHash(name); 
        }

        static bool AreEqualKeys(std::wstring const & left, std::wstring const & right)
        {
            return (left == right);
        }
        
    private:
        std::shared_ptr<PartitionedServiceDescriptor> psd_;
    };

    class StoreServicePsdCacheEntry : public PsdCacheEntryBase
    {
    public:
        explicit StoreServicePsdCacheEntry(PartitionedServiceDescriptor && psd) 
            : PsdCacheEntryBase(std::move(psd))
        { 
        }

        static bool ShouldUpdateUnderLock(
            StoreServicePsdCacheEntry const & existing, 
            StoreServicePsdCacheEntry const & incoming)
        {
            UNREFERENCED_PARAMETER(existing);
            UNREFERENCED_PARAMETER(incoming);

            return true;
        }
    };

    class GatewayPsdCacheEntry : public PsdCacheEntryBase
    {
    public:
        explicit GatewayPsdCacheEntry(PartitionedServiceDescriptor && psd) 
            : PsdCacheEntryBase(std::move(psd))
            , serviceNotFoundVersion_(ServiceModel::Constants::UninitializedVersion)
        { 
        }

        explicit GatewayPsdCacheEntry(
            std::wstring const & name,
            __int64 version)
            : PsdCacheEntryBase(name)
            , serviceNotFoundVersion_(version)
        { 
        }

        __declspec(property(get=get_Version)) __int64 ServiceNotFoundVersion;
        __int64 get_Version() const { return serviceNotFoundVersion_; }

        static bool ShouldUpdateUnderLock(
            GatewayPsdCacheEntry const & existing, 
            GatewayPsdCacheEntry const & incoming)
        {
            // Update on equals to support completing refresh waiters
            return incoming.HasPsd ? (incoming.Psd->Version >= existing.Psd->Version) : false;
        }

        __int64 serviceNotFoundVersion_;
    };

    typedef std::shared_ptr<StoreServicePsdCacheEntry> StoreServicePsdCacheEntrySPtr;
    typedef Common::LruCache<std::wstring, StoreServicePsdCacheEntry> StoreServicePsdCache;

    typedef std::shared_ptr<GatewayPsdCacheEntry> GatewayPsdCacheEntrySPtr;
    typedef Common::LruCache<std::wstring, GatewayPsdCacheEntry> GatewayPsdCache;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // The unit of a client-side cache entry. 
    // Contains alls RSPs for a particular service.
    // The PSD is cached in order to map from resolution request keys
    // to partition indexes.
    //
    class LruClientCacheEntry 
        : public Common::LruCacheEntryBase<Common::NamingUri>
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
        DENY_COPY(LruClientCacheEntry)

    public:

        LruClientCacheEntry(
            Common::NamingUri const &, 
            Naming::PartitionedServiceDescriptor &&);

        LruClientCacheEntry(
            Common::NamingUri const &, 
            Naming::PartitionedServiceDescriptor const &);

        void Initialize();

        __declspec(property(get=get_Name)) Common::NamingUri const & Name;
        Common::NamingUri const & get_Name() const { return this->GetKey(); }

        __declspec(property(get=get_Description)) Naming::PartitionedServiceDescriptor const & Description;
        Naming::PartitionedServiceDescriptor const & get_Description() const { return psd_; }

        static size_t GetHash(Common::NamingUri const &);
        static bool AreEqualKeys(Common::NamingUri const &, Common::NamingUri const &);

        static bool ShouldUpdateUnderLock(
            LruClientCacheEntry const & existing, 
            LruClientCacheEntry const & incoming);

        static bool ShouldUpdateUnderLock(
            Naming::ResolvedServicePartitionSPtr const & existing, 
            Naming::ResolvedServicePartitionSPtr const & incoming);

        Common::ErrorCode TryPutOrGetRsp(
            Common::NamingUri const & fullName,
            __inout Naming::ResolvedServicePartitionSPtr &,
            LruClientCacheCallbackSPtr const & callback = LruClientCacheCallbackSPtr());

        Common::ErrorCode TryRemoveRsp(
            Naming::PartitionKey const &);

        Common::ErrorCode TryGetRsp(
            Common::NamingUri const & fullName,
            Naming::PartitionKey const &, 
            __out Naming::ResolvedServicePartitionSPtr &);

        void GetAllRsps(
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &);

        Common::AsyncOperationSPtr BeginTryGetRsp(
            Naming::PartitionKey const &, 
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndTryGetRsp(
            Common::NamingUri const & fullName,
            Common::AsyncOperationSPtr const &,
            __out bool & isFirstWaiter,
            __out Naming::ResolvedServicePartitionSPtr &);

        Common::AsyncOperationSPtr BeginTryInvalidateRsp(
            Naming::ResolvedServicePartitionSPtr const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndTryInvalidateRsp(
            Common::NamingUri const & fullName,
            Common::AsyncOperationSPtr const &,
            __out bool & isFirstWaiter,
            __out Naming::ResolvedServicePartitionSPtr &);

        Common::ErrorCode CancelWaiters(Naming::PartitionKey const &);

        Common::ErrorCode FailWaiters(Naming::PartitionKey const &, Common::ErrorCode const &);

    private:
        
        typedef Common::LruCacheWaiterAsyncOperation<ResolvedServicePartitionCacheEntry> WaiterAsyncOperation;
        typedef std::shared_ptr<WaiterAsyncOperation> WaiterAsyncOperationSPtr;
        typedef Common::LruCacheWaiterList<ResolvedServicePartitionCacheEntry> RspWaiterList;
        typedef std::shared_ptr<RspWaiterList> RspWaiterListSPtr;

        WaiterAsyncOperationSPtr AddWaiter(
            int pIndex,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        RspWaiterListSPtr TakeWaiters(int pIndex);

        void UpdateWaiters(
            int pIndex, 
            ResolvedServicePartitionCacheEntrySPtr const &);

        void UpdateWaitersAndCallback(
            int pIndex, 
            ResolvedServicePartitionCacheEntrySPtr const &,
            LruClientCacheCallbackSPtr const &);

        bool TryGetPartitionKeyFromRsp(
            Naming::ResolvedServicePartitionSPtr const &,
            __out Naming::PartitionKey &);

        Common::ErrorCode GetPartitionIndex(
            Naming::ResolvedServicePartitionSPtr const &,
            __out int &);

        Common::ErrorCode GetPartitionIndex(
            Naming::PartitionKey const &,
            __out int &);

        static Common::NamingUri::Hasher KeyHasher;

        Naming::PartitionedServiceDescriptor psd_;

        std::vector<ResolvedServicePartitionCacheEntrySPtr> rspEntries_;
        mutable Common::RwLock rspEntriesLock_;

        std::vector<RspWaiterListSPtr> waitersList_;
        mutable Common::ExclusiveLock waitersListLock_;
    };

    typedef std::shared_ptr<LruClientCacheEntry> LruClientCacheEntrySPtr;
}

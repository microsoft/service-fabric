// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Contains common logic to retrieve cached PSD or fetch/invalidate
    // from Naming if needed.
    //
    class LruClientCacheManager::CacheAsyncOperationBase 
        : public Common::AsyncOperation
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
        DENY_COPY(CacheAsyncOperationBase)

    public:
        CacheAsyncOperationBase(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Transport::FabricActivityHeader &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        CacheAsyncOperationBase(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        __declspec(property(get=get_Owner)) LruClientCacheManager & Owner;
        LruClientCacheManager & get_Owner() const { return owner_; }

        __declspec(property(get=get_Client)) FabricClientImpl & Client;
        FabricClientImpl & get_Client() const { return client_; }

        __declspec(property(get=get_Cache)) LruClientCacheManager::LruCache & Cache;
        LruClientCacheManager::LruCache & get_Cache() const { return cache_; }

        __declspec(property(get=get_Name)) Common::NamingUri const & FullName;
        Common::NamingUri const & get_Name() const { return name_; }

        __declspec(property(get=get_ActivityHeader)) Transport::FabricActivityHeader const & ActivityHeader;
        Transport::FabricActivityHeader const & get_ActivityHeader() const { return activityHeader_; }

        __declspec(property(get=get_ClientContextId)) std::wstring const & ClientContextId;
        std::wstring const & get_ClientContextId() const;

        __declspec(property(get=get_ActivityId)) Common::ActivityId const & ActivityId;
        Common::ActivityId const & get_ActivityId() const { return activityHeader_.ActivityId; }

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        __declspec(property(get=get_RemainingTimeout)) Common::TimeSpan const RemainingTimeout;
        Common::TimeSpan const get_RemainingTimeout() const { return timeoutHelper_.GetRemainingTime(); }

    protected: // Helper functions

        LruClientCacheCallbackSPtr GetCacheCallback(Common::NamingUri const & cacheEntryName) const;

        void AddActivityHeader(ClientServerRequestMessageUPtr &);

        bool HasCachedPsd() const;

        Common::NamingUri const & GetNameWithoutMembers() const;

    protected: // Workflow functions

        virtual void GetCachedPsd(Common::AsyncOperationSPtr const &);

        // Overridden by PrefixResolveServiceAsyncOperation
        //
        virtual Common::ErrorCode EndGetCachedPsd(
            Common::AsyncOperationSPtr const &,
            __out bool & isFirstWaiter,
            __out LruClientCacheEntrySPtr &);

        // Overridden by PrefixResolveServiceAsyncOperation
        //
        virtual void OnPsdCacheMiss(Common::AsyncOperationSPtr const &);

        virtual void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) = 0;

        // Overridden by PrefixResolveServiceAsyncOperation
        //
        virtual void InvalidateCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        virtual void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) = 0;

    protected:

        void OnGetCachedPsdComplete(
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        Common::NamingUri const & GetCacheEntryName(LruClientCacheEntrySPtr const &);

    private:

        void OnGetPsdComplete(
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        void OnPsdError(
            Common::ErrorCode const &,
            Common::AsyncOperationSPtr const &);

        void OnInvalidateCachedPsdComplete(
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        LruClientCacheManager & owner_;
        LruClientCacheManager::LruCache & cache_;
        FabricClientImpl & client_;

        Common::NamingUri name_;
        mutable std::shared_ptr<Common::NamingUri> nameWithoutMembersSPtr_;
        mutable Common::RwLock nameWithoutMembersLock_;

        Common::TimeoutHelper timeoutHelper_;
        Transport::FabricActivityHeader activityHeader_;
        std::wstring traceId_;
    };
}

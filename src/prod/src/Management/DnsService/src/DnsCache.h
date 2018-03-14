// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class PublicNameEntry :
        public KShared<PublicNameEntry>
    {
        K_FORCE_SHARED(PublicNameEntry);
    public:
        static void Create(
            __out PublicNameEntry::SPtr& spNode,
            __in KAllocator& allocator,
            __in KString& name
        );

    private:
        PublicNameEntry(
            __in KString& name
        );

    public:
        KString& Name() { return *_spName; }

    private:
        KString::SPtr _spName;

    public:
        KListEntry _listEntry;
    };

    class PublicNameCache
    {
        K_DENY_COPY(PublicNameCache);

    public:
        PublicNameCache(
            __in KAllocator& allocator,
            __in ULONG maxSize
        );

        ~PublicNameCache();

    public:
        bool Contains(__in KString& name);
        void Add(__in KString& name);
        void Remove(__in KString& name);

    private:
        KAllocator& _allocator;
        ULONG _maxSize;
        KNodeList<PublicNameEntry> _lru;
        KHashTable<KString::SPtr, PublicNameEntry::SPtr> _ht;
    };

    class DnsCache :
        public KShared<DnsCache>,
        public IDnsCache
    {
        K_FORCE_SHARED(DnsCache);
        K_SHARED_INTERFACE_IMP(IDnsCache);

    public:
        static void Create(
            __out DnsCache::SPtr& spCache,
            __in KAllocator& allocator,
            __in ULONG maxSize
        );

    private:
        DnsCache(
            __in ULONG maxSize
        );

    public:
        virtual DnsNameType Read(
            __in KString& dnsName,
            __out KString::SPtr& serviceName
        ) override;

        virtual void MarkNameAsPublic(
            __in KString& serviceName
        ) override;

        virtual void Put(
            __in KString& dnsName,
            __in KString& serviceName
        ) override;

        virtual void Remove(
            __in KString& serviceName
        ) override;

        virtual bool IsServiceKnown(
            __in KString& serviceName
        ) override;

        virtual bool IsDnsNameKnown(
            __in KString& dnsName
        ) override;

        virtual void RegisterNotification(
            __in IDnsCacheNotification& notification
        ) override;

        virtual void UnregisterNotification(
            __in IDnsCacheNotification& notification
        ) override;

    private:
        void EnsureSize();

    private:
        KReaderWriterSpinLock _lock;
        PublicNameCache _publicCache;
        KHashTable<KString::SPtr, KString::SPtr> _htDnsToFabric;
        KHashTable<KString::SPtr, KString::SPtr> _htFabricToDns;

        KReaderWriterSpinLock _lockNotifications;
        KArray<IDnsCacheNotification::SPtr> _arrNotifications;
    };
}

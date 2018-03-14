// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "DnsCache.h"

class StackSharedLock
{
public:
    StackSharedLock(KReaderWriterSpinLock& lock) : _lock(lock)
    {
        _lock.AcquireShared();
    }

    ~StackSharedLock()
    {
        _lock.ReleaseShared();
    }
private:
    KReaderWriterSpinLock& _lock;
};

class StackExLock
{
public:
    StackExLock(KReaderWriterSpinLock& lock) : _lock(lock)
    {
        _lock.AcquireExclusive();
    }

    ~StackExLock()
    {
        _lock.ReleaseExclusive();
    }
private:
    KReaderWriterSpinLock& _lock;
};

void DNS::CreateDnsCache(
    __out IDnsCache::SPtr& spCache,
    __in KAllocator& allocator,
    __in ULONG maxSize
)
{
    DnsCache::SPtr sp;
    DnsCache::Create(/*out*/sp, allocator, maxSize);

    spCache = sp.RawPtr();
}

/*static*/
void DnsCache::Create(
    __out DnsCache::SPtr& spCache,
    __in KAllocator& allocator,
    __in ULONG maxSize
)
{
    spCache = _new(TAG, allocator) DnsCache(maxSize);
    KInvariant(spCache != nullptr);
}

DnsCache::DnsCache(
    __in ULONG maxSize
) :
_htDnsToFabric(997, K_DefaultHashFunction, CompareKString, GetThisAllocator()),
_htFabricToDns(997, K_DefaultHashFunction, CompareKString, GetThisAllocator()),
_publicCache(GetThisAllocator(), maxSize),
_arrNotifications(GetThisAllocator())
{
}

DnsCache::~DnsCache()
{
}

void DnsCache::Remove(
    __in KString& serviceName
)
{
    StackExLock lock(_lock);

    KString::SPtr spDnsName;
    KString::SPtr spServiceName(&serviceName);
    NTSTATUS status = _htFabricToDns.Get(spServiceName, /*out*/spDnsName);
    if (status == STATUS_SUCCESS)
    {
        _htFabricToDns.Remove(spServiceName);
        _htDnsToFabric.Remove(spDnsName);
    }
}

bool DnsCache::IsDnsNameKnown(
    __in KString& dnsName
)
{
    StackSharedLock lock(_lock);

    KString::SPtr spKey(&dnsName);
    return !!_htDnsToFabric.ContainsKey(spKey);
}

bool DnsCache::IsServiceKnown(
    __in KString& serviceName
)
{
    StackSharedLock lock(_lock);

    KString::SPtr spKey(&serviceName);
    return !!_htFabricToDns.ContainsKey(spKey);
}

DnsNameType DnsCache::Read(
    __in KString& dnsName,
    __out KString::SPtr& serviceName
)
{
    {
        StackSharedLock lock(_lock);

        // Check if the name is in fabric

        KString::SPtr spKey(&dnsName);
        NTSTATUS status = _htDnsToFabric.Get(spKey, /*out*/serviceName);
        if (status == STATUS_SUCCESS)
        {
            return DnsNameTypeFabric;
        }
    }

    // Check if this is known public name
    {
        // Has to be under Exclusive lock.
        // PublicCache is LRU cache. It gets updated on cache hit.
        StackExLock lock(_lock);

        if (_publicCache.Contains(dnsName))
        {
            return DnsNameTypePublic;
        }
    }

    return DnsNameTypeUnknown;
}

void DnsCache::MarkNameAsPublic(
    __in KString& dnsName
)
{
    StackExLock lock(_lock);

    // Only do it if the name is NOT in the fabric dictionary
    // Service Fabric notifications take precedence, always
    KString::SPtr spKey(&dnsName);
    KString::SPtr spServiceName;
    NTSTATUS status = _htDnsToFabric.Get(spKey, /*out*/spServiceName);
    if (status == STATUS_SUCCESS)
    {
        return;
    }

    _publicCache.Add(dnsName);
}

void DnsCache::Put(
    __in KString& dnsName,
    __in KString& serviceName
)
{
    // Update cache
    {
        StackExLock lock(_lock);

        _publicCache.Remove(dnsName);

        KString::SPtr spDns(&dnsName);
        KString::SPtr spFabric(&serviceName);
        NTSTATUS status = _htDnsToFabric.Put(spDns, spFabric, TRUE/*forceUpdate*/);
        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
        {
            KInvariant(false);
        }

        status = _htFabricToDns.Put(spFabric, spDns, TRUE/*forceUpdate*/);
        if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
        {
            KInvariant(false);
        }

        EnsureSize();
    }

    // Execute notifications
    {
        StackSharedLock lock(_lockNotifications);

        for (ULONG i = 0; i < _arrNotifications.Count(); i++)
        {
            _arrNotifications[i]->OnDnsCachePut(dnsName);
        }
    }
}

void DnsCache::RegisterNotification(
    __in IDnsCacheNotification& notification
)
{
    StackExLock lock(_lockNotifications);

    for (ULONG i = 0; i < _arrNotifications.Count(); i++)
    {
        if (_arrNotifications[i].RawPtr() == &notification)
        {
            return;
        }
    }
    if (STATUS_SUCCESS != _arrNotifications.Append(&notification))
    {
        KInvariant(false);
    }
}

void DnsCache::UnregisterNotification(
    __in IDnsCacheNotification& notification
)
{
    StackExLock lock(_lockNotifications);

    for (ULONG i = 0; i < _arrNotifications.Count(); i++)
    {
        if (_arrNotifications[i].RawPtr() == &notification)
        {
            _arrNotifications.Remove(i);
            return;
        }
    }
}

void DnsCache::EnsureSize()
{
    static double IncreaseFactor = 1.5;
    static double SaturationLimit = 0.6;

    double saturation = static_cast<double>(_htDnsToFabric.Count()) / static_cast<double>(_htDnsToFabric.Size());
    if (saturation > SaturationLimit)
    {
        _htDnsToFabric.Resize(static_cast<ULONG>(_htDnsToFabric.Size() * IncreaseFactor));
    }

    saturation = static_cast<double>(_htFabricToDns.Count()) / static_cast<double>(_htFabricToDns.Size());
    if (saturation > SaturationLimit)
    {
        _htFabricToDns.Resize(static_cast<ULONG>(_htFabricToDns.Size() * IncreaseFactor));
    }
}

/*static*/
void PublicNameEntry::Create(
    __out PublicNameEntry::SPtr& spNode,
    __in KAllocator& allocator,
    __in KString& name
)
{
    spNode = _new(TAG, allocator) PublicNameEntry(name);
    KInvariant(spNode != nullptr);
}

PublicNameEntry::PublicNameEntry(
    __in KString& name
) : _spName(&name)
{
}

PublicNameEntry::~PublicNameEntry()
{
}

PublicNameCache::PublicNameCache(
    __in KAllocator& allocator,
    __in ULONG maxSize
) : _allocator(allocator),
_maxSize(maxSize),
_ht(maxSize, K_DefaultHashFunction, CompareKString, allocator),
_lru(FIELD_OFFSET(PublicNameEntry, _listEntry))
{
}

PublicNameCache::~PublicNameCache()
{
    _lru.Reset();
    _ht.Clear();
}

bool PublicNameCache::Contains(__in KString& name)
{
    KString::SPtr spKey(&name);
    PublicNameEntry::SPtr spValue;
    NTSTATUS status = _ht.Get(spKey, /*out*/spValue);
    if (status == STATUS_SUCCESS)
    {
        _lru.Remove(spValue.RawPtr());
        _lru.InsertHead(spValue.RawPtr());
        return true;
    }

    return false;
}

void PublicNameCache::Add(__in KString& name)
{
    if (Contains(name))
    {
        return;
    }

    PublicNameEntry::SPtr spEntry;
    PublicNameEntry::Create(/*out*/spEntry, _allocator, name);

    KString::SPtr spKey(&name);
    NTSTATUS status = _ht.Put(spKey, spEntry, TRUE/*forceUpdate*/);
    if (status != STATUS_SUCCESS && status != STATUS_OBJECT_NAME_EXISTS)
    {
        KInvariant(false);
    }

    _lru.InsertHead(spEntry.RawPtr());

    if (_ht.Count() > _maxSize)
    {
        PublicNameEntry* entry = _lru.RemoveTail();
        KString::SPtr spKeyRemove(&entry->Name());
        _ht.Remove(spKeyRemove);
    }
}

void PublicNameCache::Remove(__in KString& name)
{
    KString::SPtr spKey(&name);
    PublicNameEntry::SPtr spValue;
    NTSTATUS status = _ht.Get(spKey, /*out*/spValue);
    if (status == STATUS_SUCCESS)
    {
        _lru.Remove(spValue.RawPtr());
        _ht.Remove(spKey);
    }
}

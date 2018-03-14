// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

class RuntimeRegistrationTable::Entry
{
    DENY_COPY(Entry)

public:
    Entry(FabricRuntimeContext const& runtimeContext)
        : RuntimeContext(runtimeContext),
        IsValid(false)
    {
    }

    void WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("RuntimeRegistrationTable::Entry { ");
        w.Write("IsValid = {0}", IsValid);
        w.Write("Registration = {0}", RuntimeContext);
        w.Write("}");
    }

public:
    FabricRuntimeContext RuntimeContext;
    bool IsValid;
};

RuntimeRegistrationTable::RuntimeRegistrationTable(
    ComponentRoot const & root)
    : RootedObject(root),
    isClosed_(false),
    lock_(),
    map_(),
    hostIndex_(),
    servicePackageIndex_(),
    codePackageIndex_()
{
}

RuntimeRegistrationTable::~RuntimeRegistrationTable()
{
}

ErrorCode RuntimeRegistrationTable::Add(FabricRuntimeContext const & runtimeContext)
{
    auto entry = make_shared<Entry>(runtimeContext);
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(runtimeContext.RuntimeId);
        if (iter != map_.end())
        {
            if (iter->second->IsValid)
            {
                return ErrorCode(ErrorCodeValue::HostingFabricRuntimeAlreadyRegistered);
            }
            else
            {
                // TODO: provide better error code
                // race between incomplete first processing and a registration request retry
                return ErrorCode(ErrorCodeValue::OperationFailed);
            }
        }
    
        AddEntryTx_CallerHoldsLock(entry);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::Validate(
    wstring const & runtimeId, 
    __out RuntimeRegistrationSPtr & registration)
{
     {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(runtimeId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
        }

        iter->second->IsValid = true;
        registration = make_shared<RuntimeRegistration>(iter->second->RuntimeContext);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::Find(
    wstring const & runtimeId, 
    __out RuntimeRegistrationSPtr & registration)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(runtimeId);
        if ((iter == map_.end()) || (!iter->second->IsValid))
        {
            return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
        }

        registration = make_shared<RuntimeRegistration>(iter->second->RuntimeContext);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::Find(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    __out vector<RuntimeRegistrationSPtr> & registrations)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto servicePackageIndexIter = servicePackageIndex_.find(servicePackageInstanceId);
        if (servicePackageIndexIter == servicePackageIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = servicePackageIndexIter->second.begin();
            entryIter != servicePackageIndexIter->second.end();
            ++entryIter)
        {
            if ((*entryIter)->IsValid)
            {
                registrations.push_back(
                    make_shared<RuntimeRegistration>((*entryIter)->RuntimeContext));           
            }
        }            
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::Find(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    __out vector<RuntimeRegistrationSPtr> & registrations)
{
    {
        AcquireReadLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto codePackageIndexIter = codePackageIndex_.find(codePackageInstanceId);
        if (codePackageIndexIter == codePackageIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = codePackageIndexIter->second.begin();
            entryIter != codePackageIndexIter->second.end();
            ++entryIter)
        {
            if ((*entryIter)->IsValid)
            {
                registrations.push_back(
                    make_shared<RuntimeRegistration>((*entryIter)->RuntimeContext));           
            }
        }            
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::Remove(wstring const & runtimeId, __out RuntimeRegistrationSPtr & registration)
{
    EntrySPtr removed;
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto iter = map_.find(runtimeId);
        if (iter == map_.end())
        {
            return ErrorCode(ErrorCodeValue::HostingFabricRuntimeNotRegistered);
        }

        removed = iter->second;
        
        // remove entry from the main map 
        map_.erase(iter);

        // remove host index on this entry
        this->RemoveFromHostIndex_CallerHoldsLock(removed);

        // remove service package index on this entry
        this->RemoveFromServicePackageIndex_CallerHoldsLock(removed);

        // remove code package index on this entry
        this->RemoveFromCodePackageIndex_CallerHoldsLock(removed);

        if (removed->IsValid)
        {
            registration = make_shared<RuntimeRegistration>(removed->RuntimeContext);
        }
        else
        {
            registration = RuntimeRegistrationSPtr();
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::RemoveRegistrationsFromHost(
    wstring const & hostId, 
    __out vector<RuntimeRegistrationSPtr> & registrations)
{
    vector<EntrySPtr> removed;

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto hostIndexIter = hostIndex_.find(hostId);
        if (hostIndexIter == hostIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = hostIndexIter->second.begin();
            entryIter != hostIndexIter->second.end();
            ++entryIter)
        {
            auto entry = *entryIter;
            removed.push_back(entry);

            // remove from main map
            this->RemoveFromMainMap_CallersHoldsLock(entry);

            // remove from service package index
            this->RemoveFromServicePackageIndex_CallerHoldsLock(entry);

            // remove from code package index
            this->RemoveFromCodePackageIndex_CallerHoldsLock(entry);
        }

        // remove all entries for this hostid from hostindex
        hostIndex_.erase(hostIndexIter);

        for(auto iter = removed.begin(); iter != removed.end(); ++iter)
        {
            if ((*iter)->IsValid)
            {
                registrations.push_back(
                    make_shared<RuntimeRegistration>((*iter)->RuntimeContext));
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::RemoveRegistrationsFromServicePackage(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    int64 servicePackageInstanceSeqNum,
    __out vector<RuntimeRegistrationSPtr> & registrations)
{
     vector<EntrySPtr> removed;
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto servicePackageIndexIter = servicePackageIndex_.find(servicePackageInstanceId);
        if (servicePackageIndexIter == servicePackageIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = servicePackageIndexIter->second.begin();
            entryIter != servicePackageIndexIter->second.end();
            ++entryIter)
        {
            auto entry = *entryIter;

            if (entry->RuntimeContext.CodeContext.ServicePackageInstanceSeqNum == servicePackageInstanceSeqNum)
            {
                removed.push_back(entry);

                // remove from main map
                this->RemoveFromMainMap_CallersHoldsLock(entry);

                // remove from host ndex 
                this->RemoveFromHostIndex_CallerHoldsLock(entry);

                // remove from code package index
                this->RemoveFromCodePackageIndex_CallerHoldsLock(entry);
            }
        }

        for(auto iter = removed.begin(); iter != removed.end(); ++iter)
        {
            this->RemoveFromServicePackageIndex_CallerHoldsLock(*iter);

            if ((*iter)->IsValid)
            {
                registrations.push_back(
                    make_shared<RuntimeRegistration>((*iter)->RuntimeContext));
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::RemoveRegistrationsFromCodePackage(
    CodePackageInstanceIdentifier const & codePackageInstanceId,
    int64 codePackageInstanceSeqNum,
    __out vector<RuntimeRegistrationSPtr> & registrations)
{
    vector<EntrySPtr> removed;
    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto codePackageIndexIter = codePackageIndex_.find(codePackageInstanceId);
        if (codePackageIndexIter == codePackageIndex_.end())
        {
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = codePackageIndexIter->second.begin();
            entryIter != codePackageIndexIter->second.end();
            ++entryIter)
        {
            auto entry = *entryIter;

            if (entry->RuntimeContext.CodeContext.CodePackageInstanceSeqNum == codePackageInstanceSeqNum)
            {
                removed.push_back(entry);

                // remove from main map
                this->RemoveFromMainMap_CallersHoldsLock(entry);

                // remove from host ndex 
                this->RemoveFromHostIndex_CallerHoldsLock(entry);

                // remove from service package index
                this->RemoveFromServicePackageIndex_CallerHoldsLock(entry);
            }
        }

        for(auto iter = removed.begin(); iter != removed.end(); ++iter)
        {
            this->RemoveFromCodePackageIndex_CallerHoldsLock(*iter);

            if ((*iter)->IsValid)
            {
                registrations.push_back(
                    make_shared<RuntimeRegistration>((*iter)->RuntimeContext));
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode RuntimeRegistrationTable::UpdateRegistrationsFromCodePackage(
    CodePackageContext const & existingContext,
    CodePackageContext const & updatedContext)
{
    ASSERT_IF(existingContext.CodePackageInstanceId != updatedContext.CodePackageInstanceId,
        "UpdateRegistrationsFromCodePackage: CodePackageIds must be same for the Update. Existing={0}, Updated={1}",
        existingContext.CodePackageInstanceId,
        updatedContext.CodePackageInstanceId);

    {
        AcquireWriteLock lock(lock_);
        if (isClosed_) { return ErrorCode(ErrorCodeValue::ObjectClosed); }

        auto codePackageIndexIter = codePackageIndex_.find(existingContext.CodePackageInstanceId);
        if (codePackageIndexIter == codePackageIndex_.end())
        {
            // there are not registrations of runtime from the specified code package
            return ErrorCode(ErrorCodeValue::Success);
        }

        for(auto entryIter = codePackageIndexIter->second.begin();
            entryIter != codePackageIndexIter->second.end();
            ++entryIter)
        {
            auto entry = *entryIter;
            if (entry->RuntimeContext.CodeContext == existingContext)
            {
                entry->RuntimeContext.CodeContext = updatedContext;
            }
        }
    }
    
    return ErrorCode(ErrorCodeValue::Success);
}

vector<RuntimeRegistrationSPtr> RuntimeRegistrationTable::Close()
{
    vector<RuntimeRegistrationSPtr> removedRegistrations;
    {
        AcquireWriteLock lock(lock_);
        if (!isClosed_)
        {
            for(auto entryIter = map_.begin(); entryIter != map_.end(); ++entryIter)
            {
                if (entryIter->second->IsValid)
                {
                    removedRegistrations.push_back(
                        make_shared<RuntimeRegistration>(entryIter->second->RuntimeContext));
                }
            }

            map_.clear();
            hostIndex_.clear();
            servicePackageIndex_.clear();

            isClosed_ = true;
        }
    }
    
    return removedRegistrations;
}

void RuntimeRegistrationTable::AddEntryTx_CallerHoldsLock(EntrySPtr const & entry)
{
    // ensure that all of the indexes have space for the new entry, if exception occurs here
    // we may have some space left over in the table but the table will be consistent 
    HostIdIndexMap::iterator hostIndexIter = hostIndex_.find(entry->RuntimeContext.HostContext.HostId);
    if (hostIndexIter == hostIndex_.end())
    {
        hostIndexIter = hostIndex_.insert(
            hostIndexIter, 
            make_pair(entry->RuntimeContext.HostContext.HostId, vector<EntrySPtr>()));
    }

    auto servicePkgInstanceId = entry->RuntimeContext.CodeContext.CodePackageInstanceId.ServicePackageInstanceId;
    auto servicePackageIndexIter = servicePackageIndex_.find(servicePkgInstanceId);
    if (servicePackageIndexIter == servicePackageIndex_.end())
    {
        servicePackageIndexIter = servicePackageIndex_.insert(
            servicePackageIndexIter,
            make_pair(servicePkgInstanceId, vector<EntrySPtr>()));
    }

    auto codePackageIndexIter = codePackageIndex_.find(entry->RuntimeContext.CodeContext.CodePackageInstanceId);
    if (codePackageIndexIter == codePackageIndex_.end())
    {
        codePackageIndexIter = codePackageIndex_.insert(
            codePackageIndexIter,
            make_pair(entry->RuntimeContext.CodeContext.CodePackageInstanceId, vector<EntrySPtr>()));
    }

    EntryMap::iterator newEntryIter = map_.insert(map_.end(), make_pair(entry->RuntimeContext.RuntimeId, entry));
    vector<EntrySPtr>::iterator hostIndexEntryIter = hostIndexIter->second.end();
    vector<EntrySPtr>::iterator servicePackageIndexEntryIter = servicePackageIndexIter->second.end();
    vector<EntrySPtr>::iterator codePackageIndexEntryIter = codePackageIndexIter->second.end();
    try
    {
        // add index entries, if they fail, the main entry and index entries will be cleaned
        hostIndexEntryIter = hostIndexIter->second.insert(hostIndexEntryIter, entry);
        servicePackageIndexEntryIter = servicePackageIndexIter->second.insert(servicePackageIndexEntryIter, entry);
        codePackageIndexEntryIter = codePackageIndexIter->second.insert(codePackageIndexEntryIter, entry);
    }
    catch(...)
    {
        
        map_.erase(newEntryIter);
        if (hostIndexEntryIter != hostIndexIter->second.end())
        {
            hostIndexIter->second.erase(hostIndexEntryIter);
        }
        if (servicePackageIndexEntryIter != servicePackageIndexIter->second.end())
        {
            servicePackageIndexIter->second.erase(servicePackageIndexEntryIter);
        }
        if (codePackageIndexEntryIter != codePackageIndexIter->second.end())
        {
            codePackageIndexIter->second.erase(codePackageIndexEntryIter);
        }
        throw;
    }
}

void RuntimeRegistrationTable::RemoveFromMainMap_CallersHoldsLock(EntrySPtr const & entry)
{
    auto iter = map_.find(entry->RuntimeContext.RuntimeId);
    ASSERT_IF(iter == map_.end(),
        "RuntimeRegistrationEntry {0} not found in map_",
        *entry);

    map_.erase(iter);
}

void RuntimeRegistrationTable::RemoveFromHostIndex_CallerHoldsLock(EntrySPtr const & entry)
{
    auto hostIndexIter = hostIndex_.find(entry->RuntimeContext.HostContext.HostId);
    ASSERT_IF(hostIndexIter == hostIndex_.end(),
        "RuntimeRegistrationEntry {0} not found in hostIndex_",
        *entry);

    auto hostEntryIter = ::find(hostIndexIter->second.begin(), hostIndexIter->second.end(), entry);
    ASSERT_IF(hostEntryIter == hostIndexIter->second.end(),
        "RuntimeRegistrationEntry {0} not found in hostIndex_",
        *entry);
    hostIndexIter->second.erase(hostEntryIter);

    if (hostIndexIter->second.size() == 0)
    {
        hostIndex_.erase(hostIndexIter);
    }
}

void RuntimeRegistrationTable::RemoveFromServicePackageIndex_CallerHoldsLock(EntrySPtr const & entry)
{
    ServicePackageInstanceIdentifier servicePackageKey(entry->RuntimeContext.CodeContext.CodePackageInstanceId.ServicePackageInstanceId);

    auto servicePackageIndexIter = servicePackageIndex_.find(servicePackageKey);
    ASSERT_IF(servicePackageIndexIter == servicePackageIndex_.end(),
        "RuntimeRegistrationEntry {0} not find in servicePackageIndex_",
        *entry);

    auto servicePackageEntryIter = ::find(servicePackageIndexIter->second.begin(), servicePackageIndexIter->second.end(), entry);
    ASSERT_IF(servicePackageEntryIter == servicePackageIndexIter->second.end(),
        "RuntimeRegistrationEntry {0} not found in servicePackageIndex_",
        *entry);
    servicePackageIndexIter->second.erase(servicePackageEntryIter);

    if (servicePackageIndexIter->second.size() == 0)
    {
        servicePackageIndex_.erase(servicePackageIndexIter);
    }
}

void RuntimeRegistrationTable::RemoveFromCodePackageIndex_CallerHoldsLock(EntrySPtr const & entry)
{
    CodePackageInstanceIdentifier codePackageKey(entry->RuntimeContext.CodeContext.CodePackageInstanceId);

    auto codePackageIndexIter = codePackageIndex_.find(codePackageKey);
    ASSERT_IF(codePackageIndexIter == codePackageIndex_.end(),
        "RuntimeRegistrationEntry {0} not find in codePackageIndex_",
        *entry);

    auto codePackageEntryIter = ::find(codePackageIndexIter->second.begin(), codePackageIndexIter->second.end(), entry);
    ASSERT_IF(codePackageEntryIter == codePackageIndexIter->second.end(),
        "RuntimeRegistrationEntry {0} not found in codePackageIndex_",
        *entry);
    codePackageIndexIter->second.erase(codePackageEntryIter);

    if (codePackageIndexIter->second.size() == 0)
    {
        codePackageIndex_.erase(codePackageIndexIter);
    }
}

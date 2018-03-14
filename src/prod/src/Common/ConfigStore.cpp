// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

#define TraceType "ConfigStore"

wstring ConfigStoreType::ToString(Enum const & val)
{
    switch(val)
    {
    case ConfigStoreType::Cfg:
        return L"Cfg";
    case ConfigStoreType::Package:
        return L"Package";
    case ConfigStoreType::SettingsFile:
        return L"SettingsFile";
    case ConfigStoreType::None:
        return L"None";
    default:
        Assert::CodingError("Unknown ConfigStoreType value {0}", (int)val);
    }
}

void ConfigStoreType::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    w << ToString(val);
}

ConfigStore::ConfigStore()
    : enable_shared_from_this<ConfigStore>(),
    ignoreUpdateFailures_(false)
{
    StringWriter writer(traceId_);
    writer.Write("{0}", TextTraceThis);
}

ConfigStore::ConfigStore(bool ignoreUpdateFailures)
    : enable_shared_from_this<ConfigStore>(),
    ignoreUpdateFailures_(ignoreUpdateFailures)
{
    StringWriter writer(traceId_);
    writer.Write("{0}", TextTraceThis);
}

ConfigStore::~ConfigStore()
{
}

bool ConfigStore::GetIgnoreUpdateFailures() const
{
    AcquireReadLock lock(lock_);
    return this->ignoreUpdateFailures_;
}

void ConfigStore::SetIgnoreUpdateFailures(bool value)
{
    AcquireWriteLock lock(lock_);
    this->ignoreUpdateFailures_ = value;
}

void ConfigStore::RegisterForUpdate(wstring const & section, IConfigUpdateSink* sink)
{
    {
        AcquireWriteLock lock(lock_);
        updateSubscriberMap_.Register(section, sink);
    }

    ConfigEventSource::Events.RegisteredForUpdate(traceId_, section, sink->GetTraceId());
}

void ConfigStore::UnregisterForUpdate(IConfigUpdateSink const * sink)
{
    vector<wstring> sections;
    {
        AcquireWriteLock lock(lock_);
        sections = updateSubscriberMap_.UnRegister(sink);
    }

    for (auto const & it : sections)
    {
        ConfigEventSource::Events.UnregisteredForUpdate(traceId_, it, sink->GetTraceId());
    }
}

bool ConfigStore::OnUpdate(std::wstring const & section, std::wstring const & key)
{
    auto result = InvokeOnSubscribers(
        section,
        [&section, &key](IConfigUpdateSink * sink) { return sink->OnUpdate(section, key); });

    WriteInfo(TraceType, traceId_, "OnUpdate for {0}/{1}. Result:{2}", section, key, result);

    if (!result)
    {
        if (this->ignoreUpdateFailures_)
        {
            WriteWarning(TraceType, traceId_, "Configuration can not be updated dynamically for {0}/{1}", section, key);
            WriteWarning(TraceType, traceId_, "Continuing with previously loaded value, this may result in invalid combination of settings, process restart is recommended.", section, key);
        }
        else
        {
            WriteError(TraceType, traceId_, "Configuration can not be updated dynamically for {0}/{1}", section, key);
            WriteError(TraceType, traceId_, "Exiting the process as configuration can not be updated dynamically.");

            // Calling TerminateProcess since ExitProcess can cause the process to stop responding
            ProcessHandle thisProcess(::GetCurrentProcess(), false);
            ::TerminateProcess(thisProcess.Value, ErrorCodeValue::UpgradeFailed & 0x0000FFFF);
        }
    }

    return result;
}

bool ConfigStore::CheckUpdate(std::wstring const & section, std::wstring const & key, std::wstring const & value, bool isEncrypted)
{    
    auto result = InvokeOnSubscribers(
        section,
        [&section, &key, &value, &isEncrypted](IConfigUpdateSink * sink) { return sink->CheckUpdate(section, key, value, isEncrypted); });

    WriteInfo(TraceType, traceId_, "CheckUpdate {0}/{1}:{2} isEncrypted: {3} result: {4}", section, key, value, isEncrypted, result);

    return result;
}

bool ConfigStore::InvokeOnSubscribers(
    std::wstring const & section,
    std::function<bool(IConfigUpdateSink *)> const & func) const
{
    bool updateFailureEncountered = false;
    IConfigUpdateSinkList impactedRegistrations;

    {
        AcquireExclusiveLock lock(lock_);
        impactedRegistrations = updateSubscriberMap_.Get(section);
    }

    for (auto & it : impactedRegistrations)
    {
        auto registration = it.lock();
        if (registration == nullptr)
        {
            continue;
        }

        bool success = func(registration.get());
        if (!success)
        {
            updateFailureEncountered = true;
        }

        WriteInfo(TraceType, traceId_, "Invoked on Subscriber: {0}. Result:{1}", registration->GetTraceId(), success);
    }

    return !updateFailureEncountered;
}

void ConfigStore::UpdateSubscriberMap::Register(std::wstring const & section, IConfigUpdateSink * sink)
{
    auto iter = map_.find(section);
    while ((iter != map_.end()) && (iter->first == section))
    {
        ASSERT_IF(*(iter->second) == sink, "sink {0} is already registered on section {1}", TextTracePtr(sink), section);
        ++iter;
    }

    map_.insert(iter, make_pair(section, make_shared<ConfigUpdateSinkRegistration>(sink)));
}

vector<wstring> ConfigStore::UpdateSubscriberMap::UnRegister(IConfigUpdateSink const* sink)
{
    vector<wstring> rv;

    auto iter = map_.begin();
    while (iter != map_.end())
    {
        if ((*iter->second) == sink)
        {
            rv.push_back(iter->first);
            auto toRemove = iter++;
            map_.erase(toRemove);
            continue;
        }

        ++iter;
    }

    return rv;
}

IConfigUpdateSinkList ConfigStore::UpdateSubscriberMap::Get(std::wstring const & section) const
{
    IConfigUpdateSinkList rv;

    auto iter = map_.begin();
    while (iter != map_.end())
    {
        if (iter->first.empty() || (iter->first == section))
        {
            rv.push_back(iter->second);
        }

        ++iter;
    }

    return rv;
}

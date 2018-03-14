// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DEFINE_SINGLETON_COMPONENT_CONFIG(FabricHostConfig)

void FabricHostConfig::Initialize()
{
    registeredForUpdates_ = false;
}

void FabricHostConfig::RegisterForUpdates(std::wstring const & partialSectionName, HostedServiceActivationManager* serviceActivator)
{
    AcquireWriteLock lock(lock_);
    auto iter = updateSubscriberMap_.find(partialSectionName);
    while((iter != updateSubscriberMap_.end()) && (iter->first == partialSectionName))
    {
        ASSERT_IF(iter->second == serviceActivator, "serviceActivator is already registered for section {0}", partialSectionName);
        ++ iter;
    }

    updateSubscriberMap_.insert(std::make_pair(partialSectionName, serviceActivator));
    if(!registeredForUpdates_)
    {
        Trace.WriteInfo("FabricHostConfig.RegisterForUpdates", "Registering hostedservice activation manager");
        registeredForUpdates_ = true;
        this->config_.RegisterForUpdate(L"", this);
    }
}

void FabricHostConfig::UnregisterForUpdates(HostedServiceActivationManager* serviceActivator)
{
    AcquireWriteLock lock(lock_);
    auto iter = updateSubscriberMap_.begin();
    while(iter != updateSubscriberMap_.end())
    {
        if (iter->second == serviceActivator)
        {
            auto toRemove = iter++;
            updateSubscriberMap_.erase(toRemove);
            continue;
        }
        ++ iter;
    }
}

void FabricHostConfig::GetSections(StringCollection & sectionNames, std::wstring const & partialName) const
{
    this->config_.GetSections(sectionNames, partialName);
}

void FabricHostConfig::GetKeyValues(std::wstring const & section, StringMap & entries) const
{
    this->config_.GetKeyValues(section, entries);
}

bool FabricHostConfig::IsConfigSettingEncrypted(wstring const & section, wstring const & key) const
{
    bool isConfigSettingEncrypted;
    this->config_.ReadString(section, key, isConfigSettingEncrypted);

    return isConfigSettingEncrypted;
}

bool FabricHostConfig::OnUpdate(std::wstring const & section, std::wstring const & key)
{
    Trace.WriteInfo("FabricHostConfig.OnUpdate", "Handling update on section {0} key {1}", section, key);
    bool retval = this->ComponentConfig::OnUpdate(section, key);
    {
        AcquireReadLock lock(lock_);
        for(auto it = updateSubscriberMap_.begin(); it != updateSubscriberMap_.end(); it++)
        {
            if(it->first.empty() || StringUtility::ContainsCaseInsensitive(section, it->first))
            {
                retval = true;
                it->second->OnConfigChange(section, key);
            }
        }
    }
    return retval;
}

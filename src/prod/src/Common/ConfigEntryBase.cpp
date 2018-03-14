// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ConfigEntryBase::ConfigEntryBase()
    :   componentConfig_(nullptr),
        upgradePolicy_(ConfigEntryUpgradePolicy::NotAllowed),
        initialized_(false),
        loaded_(false),
        hasValue_(false)
{
}

void ConfigEntryBase::Initialize(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, ConfigEntryUpgradePolicy::Enum upgradePolicy)
{
    if (!initialized_)
    {
        componentConfig_ = const_cast<ComponentConfig*>(componentConfig);
        section_ = section;
        key_ = key;
        upgradePolicy_ = upgradePolicy;

        componentConfig_->AddEntry(section, *this);

        initialized_ = true;
    }
}

bool ConfigEntryBase::OnUpdate()
{
    bool updated = LoadValue();
    if (updated)
    {
        if (upgradePolicy_ != ConfigEntryUpgradePolicy::Dynamic)
        {
            ComponentConfig::WriteError(componentConfig_->Name,
                "Config for {0}.{1} changed but this entry can not be dynamically updated",
                section_, key_);

            return false;
        }

        ComponentConfig::WriteInfo(componentConfig_->Name,
            "Config for {0}.{1} changed, raising event",
            section_, key_);

        event_.Fire(EventArgs(), true);

        ComponentConfig::WriteInfo(componentConfig_->Name,
            "Config for {0}.{1} updated",
            section_, key_);
    }
    else
    {
        ComponentConfig::WriteInfo(
            componentConfig_->Name,
            "Config for {0}.{1} is not updated",
            section_, key_);
    }

    return true;
}

bool ConfigEntryBase::CheckUpdate(wstring const & value, bool isEncrypted)
{
    wstring stringValue = value;
    return ((upgradePolicy_ == ConfigEntryUpgradePolicy::Dynamic) || !LoadValue(stringValue, isEncrypted, true));
}

bool ConfigEntryBase::Matches(std::wstring const & key)
{
    return (key_ == key || key_.size() == 0);
}

HHandler ConfigEntryBase::AddHandler(EventHandler const & handler)
{
    return event_.Add(handler);
}

bool ConfigEntryBase::RemoveHandler(HHandler const & hHandler)
{
    return event_.Remove(hHandler);
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

ComponentConfig::ComponentConfig(StringLiteral name)
    : configLock_(),
    config_(),
    traceId_(),
    name_(name),
    sections_()
{
}

ComponentConfig::ComponentConfig(ConfigStoreSPtr const & store, StringLiteral name)
    : configLock_(),
    config_(store),
    traceId_(),
    name_(name),
    sections_()
{
}


ComponentConfig::~ComponentConfig()
{
}

void ComponentConfig::AddEntry(wstring const & section, ConfigEntryBase & entry)
{
    vector<ComponentConfigSectionUPtr>::iterator it;
    for (it = sections_.begin(); it != sections_.end() && (*it)->Name != section; ++it);
            
    if (it == sections_.end())
    {
        ComponentConfigSectionUPtr sectionUPtr = make_unique<ComponentConfigSection>(section);
        sectionUPtr->AddEntry(&entry);
        sections_.push_back(move(sectionUPtr));
        config_.RegisterForUpdate(section, this);
    }
    else
    {
        (*it)->AddEntry(&entry);
    }
}

bool ComponentConfig::OnUpdate(wstring const & section, wstring const & key)
{
    AcquireReadLock grab(configLock_);
    for (auto it = sections_.begin(); it != sections_.end(); ++it)
    {
        (*it)->ClearCachedValue();
    }

    for (auto it = sections_.begin(); it != sections_.end(); ++it)
    {
        if ((*it)->Name == section)
        {
            return (*it)->OnUpdate(key);
        }
    }

    return true;
}

bool ComponentConfig::CheckUpdate(wstring const & section, wstring const & key, wstring const & value, bool isEncrypted)
{
    AcquireReadLock grab(configLock_);

    for (auto it = sections_.begin(); it != sections_.end(); ++it)
    {
        if ((*it)->Name == section)
        {
            return (*it)->CheckUpdate(key, value, isEncrypted);
        }
    }
    
    return true;
}

void ComponentConfig::ComponentConfigSection::AddEntry(ConfigEntryBase* entry)
{
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        if ((*it)->Key == entry->Key)
        {
            throw std::runtime_error("Duplicate configuration entries");
        }
    }

    entries_.push_back(entry);
}

void ComponentConfig::ComponentConfigSection::ClearCachedValue()
{
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        (*it)->HasValue = false;
    }
}

bool ComponentConfig::ComponentConfigSection::OnUpdate(wstring const & key)
{
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        if ((*it)->Matches(key))
        {
            return (*it)->OnUpdate();
        }
    }

    return true;
}

bool ComponentConfig::ComponentConfigSection::CheckUpdate(wstring const & key, wstring const & value, bool isEncrypted)
{
    for (auto it = entries_.begin(); it != entries_.end(); ++it)
    {
        if ((*it)->Matches(key))
        {
            return (*it)->CheckUpdate(value, isEncrypted);
        }
    }
    
    return true;
}

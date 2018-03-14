// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

// ********************************************************************************************************************
// ConfigSettingsConfigStore Implementation
//
ConfigSettingsConfigStore::ConfigSettingsConfigStore(ConfigSettings && configSettings)
    : ConfigStore(),
    lock_(),
    settings_(move(configSettings))

{
}

ConfigSettingsConfigStore::ConfigSettingsConfigStore(ConfigSettings const & configSettings)
    : ConfigStore(),
    lock_(),
    settings_(configSettings)
{
    settings_ = configSettings;
}

ConfigSettingsConfigStore::~ConfigSettingsConfigStore()
{
}

void ConfigSettingsConfigStore::GetSections(StringCollection & sectionNames, wstring const & partialName) const
{
    AcquireReadLock lock(lock_);

    for(auto iter = this->settings_.Sections.begin();
        iter != this->settings_.Sections.end();
        ++iter)
    {
        if (StringUtility::ContainsCaseInsensitive(iter->first, partialName))
        {
            sectionNames.push_back(iter->first);
        }
    }
}

void ConfigSettingsConfigStore::GetKeys(wstring const & sectionName, StringCollection & keyNames, wstring const & partialName) const
{
    AcquireReadLock lock(lock_);
    
    auto sectionIter = settings_.Sections.find(sectionName);
    if (sectionIter != settings_.Sections.end())
    {
        for(auto iter = sectionIter->second.Parameters.begin();
            iter != sectionIter->second.Parameters.end();
            ++iter)
        {
            if (StringUtility::ContainsCaseInsensitive(iter->first, partialName))
            {
                keyNames.push_back(iter->first);
            }
        }
    }
}

wstring ConfigSettingsConfigStore::ReadString(wstring const & section, wstring const & key, __out bool & isEncrypted) const
{
    AcquireReadLock lock(lock_);

    isEncrypted = false;

    auto sectionIter = settings_.Sections.find(section);
    if (sectionIter != settings_.Sections.end())
    {
        auto paramIter = sectionIter->second.Parameters.find(key);
        if (paramIter != sectionIter->second.Parameters.end())
        {
            isEncrypted = paramIter->second.IsEncrypted;
            return paramIter->second.Value;
        }
    }

    return L"";
}

void ConfigSettingsConfigStore::Set(wstring const & sectionName, ConfigParameter && parameter)
{
    AcquireWriteLock lock(lock_);
    settings_.Set(sectionName, move(parameter));
}

void ConfigSettingsConfigStore::Remove(wstring const & sectionName)
{
    AcquireWriteLock lock(lock_);
    settings_.Remove(sectionName);
}

bool ConfigSettingsConfigStore::Update(ConfigSettings const & updatedSettings)
{
    vector<pair<wstring, wstring>> changes;

    {
        AcquireWriteLock lock(lock_);

        for(auto existingSectionIter = settings_.Sections.begin();
            existingSectionIter != settings_.Sections.end();
            ++existingSectionIter)
        {
            auto updatedSectionIter = updatedSettings.Sections.find(existingSectionIter->first);
            if (updatedSectionIter != updatedSettings.Sections.end())
            {
                // section modified
                ProcessModifiedSection(existingSectionIter->second, updatedSectionIter->second, changes);
            }
            else
            {
                // section removed
                ProcessAddedOrRemovedSection(existingSectionIter->second, changes);
            }
        }

        for(auto updatedSectionIter = updatedSettings.Sections.begin();
            updatedSectionIter != updatedSettings.Sections.end();
            ++updatedSectionIter)
        {
            if (settings_.Sections.find(updatedSectionIter->first) == settings_.Sections.end())
            {
                // section added
                ProcessAddedOrRemovedSection(updatedSectionIter->second, changes);
            }
        }

        settings_ = updatedSettings;
    }

    // dispatch changes to the base class
    bool updateResult = true;
    for(auto iter = changes.begin(); iter != changes.end(); ++iter)
    {
        if (!ConfigStore::OnUpdate(iter->first, iter->second))
        {
            updateResult = false;
        }
    }

    return updateResult;
}

void ConfigSettingsConfigStore::ProcessAddedOrRemovedSection(
    ConfigSection const & section,
    __inout vector<pair<wstring, wstring>> & changes)
{
    for(auto paramIter = section.Parameters.begin(); 
        paramIter != section.Parameters.end(); 
        ++paramIter)
    {
        changes.push_back(make_pair(section.Name, paramIter->second.Name));
    }
}

void ConfigSettingsConfigStore::ProcessModifiedSection(
    ConfigSection const & existingSection,
    ConfigSection const & updatedSection,
    __inout vector<pair<wstring, wstring>> & changes)
{
    for(auto existingParamIter = existingSection.Parameters.begin(); 
        existingParamIter != existingSection.Parameters.end(); 
        ++existingParamIter)
    {
        auto updatedParamIter = updatedSection.Parameters.find(existingParamIter->first);
        if (updatedParamIter != updatedSection.Parameters.end())
        {
            if (existingParamIter->second.Value != updatedParamIter->second.Value)
            {
                // parameter modified
                changes.push_back(make_pair(existingSection.Name, existingParamIter->second.Name));
            }
            // else parameter value is same in existing and update config
        }
        else
        {
            // parameter removed
            changes.push_back(make_pair(existingSection.Name, existingParamIter->second.Name));
        }
    }

    for(auto updatedParamIter = updatedSection.Parameters.begin(); 
        updatedParamIter != updatedSection.Parameters.end(); 
        ++updatedParamIter)
    {
        if (existingSection.Parameters.find(updatedParamIter->first) == existingSection.Parameters.end())
        {
            // parameter added
            changes.push_back(make_pair(updatedSection.Name, updatedParamIter->second.Name));
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ConfigSettingsConfigStore : public ConfigStore
    {
        DENY_COPY(ConfigSettingsConfigStore)

    public:             
        typedef std::function<void(Common::ConfigSettings &)> Preprocessor;

        ConfigSettingsConfigStore(ConfigSettings const & configSettings);
        ConfigSettingsConfigStore(ConfigSettings && configSettings);
        virtual ~ConfigSettingsConfigStore();

        virtual std::wstring ReadString(
            std::wstring const & section,
            std::wstring const & key,
            __out bool & isEncrypted) const; 
       
        virtual void GetSections(
            Common::StringCollection & sectionNames, 
            std::wstring const & partialName = L"") const;

        virtual void GetKeys(
            std::wstring const & section,
            Common::StringCollection & keyNames, 
            std::wstring const & partialName = L"") const;

        void Set(std::wstring const & sectionName, ConfigParameter && parameter);

        void Remove(std::wstring const & sectionName);

        bool Update(ConfigSettings const & updatedSettings);

    private:
        void ProcessAddedOrRemovedSection(
            ConfigSection const & section,
            __inout std::vector<std::pair<std::wstring, std::wstring>> & changes);

        void ProcessModifiedSection(
            ConfigSection const & existingSection,
            ConfigSection const & updatedSection,
            __inout std::vector<std::pair<std::wstring, std::wstring>> & changes);

        ConfigSettings settings_;
        mutable RwLock lock_;
    };
} 

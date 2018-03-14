// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ComponentConfig;

    class ConfigEntryBase
    {
        DENY_COPY(ConfigEntryBase);

    public:
        ConfigEntryBase();

        __declspec(property(get=get_Key)) std::wstring const & Key;
        std::wstring const & get_Key() const { return key_; }

        __declspec(property(get=get_Section)) std::wstring const & Section;
        std::wstring const & get_Section() const { return section_; }

        __declspec(property(get=get_HasValue, put=set_HasValue)) bool HasValue;
        bool get_HasValue() const { return hasValue_; }
        void set_HasValue(bool value) { hasValue_ = value; }

        void Initialize(ComponentConfig const * componentConfig, std::wstring const & section, std::wstring const & key, ConfigEntryUpgradePolicy::Enum upgradePolicy);

        virtual bool LoadValue() = 0;
        virtual bool LoadValue(std::wstring & stringValue, bool isEncrypted, bool isCheckOnly) = 0;

        bool Matches(std::wstring const & key);

        HHandler AddHandler(EventHandler const & handler);

        bool RemoveHandler(HHandler const & hHandler);

        bool OnUpdate();
        bool CheckUpdate(std::wstring const & value, bool isEncrypted);

    protected:
        ComponentConfig* componentConfig_;
        std::wstring section_;
        std::wstring key_;
        Event event_;
        bool initialized_;
        ConfigEntryUpgradePolicy::Enum upgradePolicy_;
        bool loaded_;
        bool hasValue_;
    };
}

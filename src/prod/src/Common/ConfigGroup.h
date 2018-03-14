// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <typename TKey, typename TValue>
    class ConfigCollection : public std::map<TKey, TValue>
    {
    public:
        static ConfigCollection Parse(StringMap const & entries)
        {
            ConfigCollection result;

            for (auto it = entries.begin(); it != entries.end(); ++it)
            {
                result[Config::Parse<TKey>(it->first)] = Config::Parse<TValue>(it->second);
            }

            return result;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            for (auto it = this->begin(); it != this->end(); ++it)
            {
                w.WriteLine("\r\n{0}:{1}", it->first, it->second);
            }
        }
    };

    template <typename T>
    class ConfigGroup : public ConfigEntryBase
    {
        DENY_COPY(ConfigGroup);

    public:
        ConfigGroup()
        {
        }

        T const & GetValue() { return value_; }

        virtual bool LoadValue(std::wstring & stringValue, bool isEncrypted, bool isCheckOnly)
        {
            UNREFERENCED_PARAMETER(stringValue);
            UNREFERENCED_PARAMETER(isEncrypted);
            UNREFERENCED_PARAMETER(isCheckOnly);

            return true;
        }

        bool LoadValue()
        {
            StringMap entries;
            componentConfig_->GetKeyValues(section_, entries);

            bool updated = (!loaded_ || entries != entries_);
            if (updated)
            {
                value_ = T::Parse(entries);
                entries_ = move(entries);
            }

            loaded_ = hasValue_ = true;

            return updated;
        }

        void Load(ComponentConfig const * componentConfig, std::wstring const & section, ConfigEntryUpgradePolicy::Enum upgradePolicy)
        {
            Initialize(componentConfig, section, L"", upgradePolicy);

            LoadValue();
        }

        void SetValue(ComponentConfig const * componentConfig, std::wstring const & section, T const & value, ConfigEntryUpgradePolicy::Enum upgradePolicy)
        {
            Initialize(componentConfig, section, L"", upgradePolicy);

            value_ = value;
            hasValue_ = true;

            ComponentConfig::WriteInfo(componentConfig_->Name,
                "Set property {0} with value {1}",
                section_, value);
        }

    private:
        T value_;
        StringMap entries_;
    };
}

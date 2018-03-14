// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ConfigurationSetter
    {
    public:
		ConfigurationSetter();
        bool SetProperty(Common::StringCollection const & params);
    private:
		std::map<std::wstring, Common::ConfigEntryBase*> map_;
		void PopulateConfigMap();
		void AddMapEntry(Common::ConfigEntryBase & entry);
		void AddMapEntry(Common::ConfigEntryBase & entry, std::wstring customKey);
		std::wstring GenerateMapKey(std::wstring const & key);
		std::wstring GenerateMapKey(std::string & key);
		void SetConfiguration(Common::ConfigEntry<int> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<double> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<Common::TimeSpan> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<bool> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<std::string> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<std::wstring> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<Common::SecureString> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<unsigned int> * configuration, std::wstring const & value);
		void SetConfiguration(Common::ConfigEntry<int64> * configuration, std::wstring const & value);
		bool ProcessCustomConfigurationSetter(StringCollection const & params);
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class FileXmlSettingsStore;
    typedef std::shared_ptr<ServiceModel::FileXmlSettingsStore> FileXmlSettingsStoreSPtr;

    class FileXmlSettingsStore : public Common::MonitoredConfigSettingsConfigStore
    {
        DENY_COPY(FileXmlSettingsStore)

    public:
        FileXmlSettingsStore(std::wstring const & fileName, ConfigSettingsConfigStore::Preprocessor const & preprocessor);
        virtual ~FileXmlSettingsStore();

        static FileXmlSettingsStoreSPtr Create(
            std::wstring const & fileName, 
            ConfigSettingsConfigStore::Preprocessor const & preprocessor = nullptr,
            bool doNotEnableChangeMonitoring = false);

    protected:
        virtual Common::ErrorCode LoadConfigSettings(__out Common::ConfigSettings & settings);

	private:
        std::wstring fileName_;

    }; // end class FileXmlSettingsStore
} // end namespace Common

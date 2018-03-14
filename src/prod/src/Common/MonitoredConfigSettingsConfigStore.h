// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class MonitoredConfigSettingsConfigStore : public ConfigSettingsConfigStore
    {
        DENY_COPY(MonitoredConfigSettingsConfigStore)

    public:
        virtual ~MonitoredConfigSettingsConfigStore();

    protected:
        MonitoredConfigSettingsConfigStore(
            std::wstring const & fileToMonitor, 
            ConfigSettingsConfigStore::Preprocessor const & preprocessor = nullptr);

    public:
        Common::ErrorCode EnableChangeMonitoring();
        Common::ErrorCode DisableChangeMonitoring();

    protected:
        virtual Common::ErrorCode LoadConfigSettings(__out Common::ConfigSettings & settings) = 0;
        void Initialize();

    private:
		Common::ErrorCode LoadConfigSettings_WithRetry(
            __out Common::ConfigSettings & settings);
        void OnFileChanged();

        static const int MaxRetryIntervalInMillis;
        static const int MaxRetryCount;

	private:
		class ChangeMonitor;
        friend class ChangeMonitor;
        std::wstring fileName_;
        std::shared_ptr<ChangeMonitor> changeMonitor_;
        ConfigSettingsConfigStore::Preprocessor const preprocessor_;
    };
} 

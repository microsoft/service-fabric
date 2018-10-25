// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // Internal representation of Transactional Replicator Settings
    class SLInternalSettings;
    typedef std::shared_ptr<SLInternalSettings> SLInternalSettingsSPtr;

    class SLInternalSettings : 
        public SLConfigBase
    {
        DENY_COPY_CONSTRUCTOR(SLInternalSettings);

    public:

        static SLInternalSettingsSPtr Create(
            KtlLoggerSharedLogSettingsUPtr && userSetting);

        // Define config properties
        DEFINE_SL_OVERRIDEABLE_CONFIG_PROPERTIES();
        
        // Define global config properties
        DEFINE_SL_GLOBAL_CONFIG_PROPERTIES();

        DEFINE_GET_SL_CONFIG_METHOD();

        std::wstring ToString() const;

        int WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
    private:

        SLInternalSettings(
            KtlLoggerSharedLogSettingsUPtr && userSettings);

        // Loads global defaults from global config and also over rides any user provided values for settings
        void LoadSettings();
        
        template<class T>
        void TraceConfigUpdate(
            __in std::wstring configName,
            __in T previousConfig,
            __in T newConfig);

        MUTABLE_RWLOCK(SLInternalSettings, lock_);

        KtlLoggerSharedLogSettingsUPtr const userSettings_;

        // Define config members
        DEFINE_SL_PRIVATE_CONFIG_MEMBERS();

        // Define global config members
        DEFINE_SL_PRIVATE_GLOBAL_CONFIG_MEMBERS();
    };
}

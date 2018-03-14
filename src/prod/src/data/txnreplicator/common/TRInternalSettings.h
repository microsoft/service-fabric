// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // Internal representation of Transactional Replicator Settings
    class TRInternalSettings;
    typedef std::shared_ptr<TRInternalSettings> TRInternalSettingsSPtr;

    class TRInternalSettings : 
        public TRConfigBase
    {
        DENY_COPY_CONSTRUCTOR(TRInternalSettings);

    public:

        static TRInternalSettingsSPtr Create(
            TransactionalReplicatorSettingsUPtr && userSettings,
            std::shared_ptr<TransactionalReplicatorConfig> const & globalConfig);

        // Define config properties
        DEFINE_TR_OVERRIDEABLE_CONFIG_PROPERTIES();
        
        // Define global config properties
        DEFINE_TR_GLOBAL_CONFIG_PROPERTIES();

        DEFINE_GET_TR_CONFIG_METHOD();

        std::wstring ToString() const;

        int WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
    private:

        TRInternalSettings(
            TransactionalReplicatorSettingsUPtr && userSettings,
            std::shared_ptr<TransactionalReplicatorConfig> const & globalConfig);

        // Loads global defaults from global config and also over rides any user provided values for settings
        void LoadSettings();

        // Loads global configs
        int LoadGlobalSettingsCallerHoldsLock ();

        // Loads the defaults from global config which are optionally overridden by a user in the CreateReplicator API
        int LoadDefaultsForDynamicOverridableSettingsCallerHoldsLock();

        int LoadDefaultsForStaticOverridableSettingsCallerHoldsLock();

        // Loads settings provided by user
        void LoadUserSettingsCallerHoldsLock();

        void LoadDefaultsFromCodeIfInvalid();

        template<class T>
        void TraceConfigUpdate(
            __in std::wstring configName,
            __in T previousConfig,
            __in T newConfig);

        MUTABLE_RWLOCK(TRInternalSettings, lock_);

        std::shared_ptr<TransactionalReplicatorConfig> const globalConfig_;
        TransactionalReplicatorSettingsUPtr const userSettings_;

        // Define config members
        DEFINE_TR_PRIVATE_CONFIG_MEMBERS();

        // Define global config members
        DEFINE_TR_PRIVATE_GLOBAL_CONFIG_MEMBERS();
    };
}

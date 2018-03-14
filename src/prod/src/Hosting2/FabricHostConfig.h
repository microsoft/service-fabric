// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricHostConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(FabricHostConfig, "FabricHostConfig")

    public:
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricHost", ActivatorServiceAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static);
        INTERNAL_CONFIG_ENTRY(std::wstring, L"FabricHost", ActivatorServerId, L"", Common::ConfigEntryUpgradePolicy::Static);        
        INTERNAL_CONFIG_ENTRY(Common::TimeSpan, L"FabricHost", HostedServiceContinuousExitFailureResetInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //The timeout for hosted service activation, deactivation and upgrade.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricHost", StopTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Timeout for fabricactivationmanager startup
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricHost", StartTimeout, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Backoff interval on every activation failure,On every continuous activation failure the system will retry the activation for up to the MaxActivationFailureCount. The retry interval on every try is a product of continuous activation failure and the activation back-off interval.
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricHost", ActivationRetryBackoffInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Dynamic);
        //Max retry interval for Activation. On every continuous failure the retry interval is calculated as Min( ActivationMaxRetryInterval, Continuous Failure Count * ActivationRetryBackoffInterval)
        PUBLIC_CONFIG_ENTRY(Common::TimeSpan, L"FabricHost", ActivationMaxRetryInterval, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is the maximum count for which system will retry failed activation before giving up. 
        PUBLIC_CONFIG_ENTRY(int, L"FabricHost", ActivationMaxFailureCount, 10, Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is to enable fabric automatic update via Windows Update.
        PUBLIC_CONFIG_ENTRY(bool, L"FabricHost", EnableServiceFabricAutomaticUpdates, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is to enable base update for server.        
        PUBLIC_CONFIG_ENTRY(bool, L"FabricHost", EnableServiceFabricBaseUpgrade, false, Common::ConfigEntryUpgradePolicy::Dynamic);
        //This is to enable server restart.
        PUBLIC_CONFIG_ENTRY(bool, L"FabricHost", EnableRestartManagement, false, Common::ConfigEntryUpgradePolicy::Dynamic);

        //Test configs
        INTERNAL_CONFIG_ENTRY(bool, L"FabricHost", Test_IgnoreHostedServices, false, Common::ConfigEntryUpgradePolicy::Static);

    public:
        void GetSections(Common::StringCollection & sectionNames, std::wstring const & partialName) const;
        void GetKeyValues(std::wstring const & section, Common::StringMap & entries) const;
        
        bool IsConfigSettingEncrypted(std::wstring const & section, std::wstring const & key) const;

        void RegisterForUpdates(std::wstring const & sectionName, HostedServiceActivationManager* serviceActivator);
        void UnregisterForUpdates(HostedServiceActivationManager* serviceActivator);

        virtual bool OnUpdate(std::wstring const & section, std::wstring const & key);
        virtual void Initialize();

    private:
        Common::RwLock lock_;
        std::multimap<std::wstring, HostedServiceActivationManager*> updateSubscriberMap_;
        bool registeredForUpdates_;
    };
}

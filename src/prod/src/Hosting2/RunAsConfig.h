// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RunAsConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(RunAsConfig, "RunAs")

        /* -------------- RunAs Section ------------------ */

        // Indicates the RunAs account name.
        // This is only needed for "DomainUser" or "ManagedServiceAccount" account type.
        // Valid values are "domain\user" or "user@domain"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs", RunAsAccountName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account type.
        // This is needed for any RunAs section
        // Valid values are "DomainUser/NetworkService/ManagedServiceAccount/LocalSystem"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs", RunAsAccountType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type. Use RunAsPassword instead
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"RunAs", RunAsAccountPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs", RunAsPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    };

    class RunAs_FabricConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(RunAs_FabricConfig, "RunAs_Fabric")

        /* -------------- RunAs Section for Fabric service ------------------ */

        // Indicates the RunAs account name.
        // This is only needed for "DomainUser" or "ManagedServiceAccount" account type.
        // Valid values are "domain\user" or "user@domain"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_Fabric", RunAsAccountName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account type.
        // This is needed for any RunAs section
        // Valid values are "LocalUser/DomainUser/NetworkService/ManagedServiceAccount/LocalSystem"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_Fabric", RunAsAccountType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type. Use RunAsPassword instead
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"RunAs_Fabric", RunAsAccountPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_Fabric", RunAsPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    };

    class RunAs_HttpGatewayConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(RunAs_HttpGatewayConfig, "RunAs_HttpGateway")

        /* -------------- RunAs  for HttpGatewayService Section ------------------ */

        // Indicates the RunAs account name.
        // This is only needed for "DomainUser" or "ManagedServiceAccount" account type.
        // Valid values are "domain\user" or "user@domain"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_HttpGateway", RunAsAccountName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account type.
        // This is needed for any RunAs section
        // Valid values are "LocalUser/DomainUser/NetworkService/ManagedServiceAccount/LocalSystem"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_HttpGateway", RunAsAccountType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type. Use RunAsPassword instead
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"RunAs_HttpGateway", RunAsAccountPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_HttpGateway", RunAsPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    };

    class RunAs_DCAConfig : public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(RunAs_DCAConfig, "RunAs_DCA")

        /* -------------- RunAs  for DCA Section ------------------ */

        // Indicates the RunAs account name.
        // This is only needed for "DomainUser" or "ManagedServiceAccount" account type.
        // Valid values are "domain\user" or "user@domain"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_DCA", RunAsAccountName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account type.
        // This is needed for any RunAs section
        // Valid values are "LocalUser/DomainUser/NetworkService/ManagedServiceAccount/LocalSystem"
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_DCA", RunAsAccountType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type. Use RunAsPassword instead
        DEPRECATED_CONFIG_ENTRY(std::wstring, L"RunAs_DCA", RunAsAccountPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        // Indicates the RunAs account password.
        // This is only needed for "DomainUser" account type.
        PUBLIC_CONFIG_ENTRY(std::wstring, L"RunAs_DCA", RunAsPassword, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}

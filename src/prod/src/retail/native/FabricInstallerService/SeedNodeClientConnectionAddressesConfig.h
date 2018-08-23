// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricInstallerService
{
    struct SeedNodeEntryConfig
    {
        SeedNodeEntryConfig()
        {
        }

        SeedNodeEntryConfig(std::wstring const & nodeName, std::wstring const & connectionString)
            : NodeName(nodeName), ConnectionString(connectionString)
        {
        }

        std::wstring NodeName;
        std::wstring ConnectionString;
    };

    struct SeedNodeConfig : public std::vector<SeedNodeEntryConfig>
    {
        static SeedNodeConfig Parse(Common::StringMap const & entries);
        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const;
    };

    class SeedNodeClientConnectionAddressesConfig :
         public Common::ComponentConfig
    {
        DECLARE_SINGLETON_COMPONENT_CONFIG(SeedNodeClientConnectionAddressesConfig, "SeedNodeClientConnectionAddresses")

        INTERNAL_CONFIG_GROUP(SeedNodeConfig, L"SeedNodeClientConnectionAddresses", SeedNodes, Common::ConfigEntryUpgradePolicy::Dynamic);
    };
}

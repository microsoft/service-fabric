// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace TokenValidationService
    {
        class TokenValidationServiceConfig : public Common::ComponentConfig
        {
            DECLARE_SINGLETON_COMPONENT_CONFIG(TokenValidationServiceConfig, "TokenValidationServiceConfig")

            // The config section names used here are only to document the possible config parameters.
            // At runtime, the actual configuration section name will be a service name that's passed to
            // the service itself via initialization parameters so that the service can read additional
            // configurations from its own section.

        public:
            //Comma separated list of token validation providers to enable (valid providers are: DSTS, AAD).
            //Currently only a single provider can be enabled at any time.
            PUBLIC_CONFIG_ENTRY(std::wstring, L"TokenValidationService", Providers, L"DSTS", Common::ConfigEntryUpgradePolicy::Static);

            //
            // "DSTSTokenValidationService" is the legacy internal service name used before multiple validation providers were supported
            //

            //DNS name of the DSTS server
            INTERNAL_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", DSTSDnsName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //Realm name of DSTS server
            INTERNAL_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", DSTSRealm, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //DNS name of cloud service for which DSTS security token is requested
            INTERNAL_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", CloudServiceDnsName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //Name of cloud service for which DSTS security token is requested
            INTERNAL_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", CloudServiceName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //X509 Certificate find value for DSTS public certificate
            DEPRECATED_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", PublicCertificateFindValue, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //X509 certificate findtype for DSTS public certificate ex. FindByThumbprint
            DEPRECATED_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", PublicCertificateFindType, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
            //Store name where DSTS servers public certificate is stored
            DEPRECATED_CONFIG_ENTRY(std::wstring, L"DSTSTokenValidationService", PublicCertificateStoreName, L"", Common::ConfigEntryUpgradePolicy::Dynamic);
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace TXRStatefulServiceBase
{
    class Helpers
    {
    public:
        static Common::GlobalWString ReplicatorSettingsConfigPackageName;
        static Common::GlobalWString ReplicatorSettingsSectionName;
        static Common::GlobalWString ServiceHttpEndpointResourceName;

        static void EnableTracing();

        static std::wstring GetFabricNodeIpAddressOrFqdn();

        static std::wstring GetWorkingDirectory();

        static Common::ComPointer<IFabricReplicatorSettingsResult> LoadV1ReplicatorSettings();
        
        // Loads default http resource in the manifest if it exists with the name given by ServiceHttpEndpointResourceName
        static ULONG GetHttpPortFromConfigurationPackage();

        // Loads http resource in the manifest with the given name in the parameter
        static ULONG GetHttpPortFromConfigurationPackage(__in std::wstring const & endpointResourceName);
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System.Fabric;
    using System.Globalization;
    using Microsoft.Win32;

    internal class TelemetryConsumerWindows : TelemetryConsumerBase
    {
        public TelemetryConsumerWindows(string applicationInstanceId)
            : base(applicationInstanceId)
        {
        }

        // code copied and pasted from infrastructurewrapper\wrappercommon\Utilities.cs
        // not re-used to not add a new dependency
        protected override string GetServiceFabricVersion()
        {
            const string FabricInstalledRegPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Service Fabric";
            const string FabricInstalledRegPathDepreciated = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows Fabric";
            const string FabricInstalledVersionRegKey = "FabricVersion";
            const string FabricV1InstalledVersionRegKey = "WinFabInstalled";

            string version = (string)Registry.GetValue(FabricInstalledRegPath, FabricInstalledVersionRegKey, null);

            if (string.IsNullOrEmpty(version))
            {
                // Check if depreciated registry key before renaming specified
                version = (string)Registry.GetValue(FabricInstalledRegPathDepreciated, FabricInstalledVersionRegKey, null);
            }

            if (string.IsNullOrEmpty(version))
            {
                // Check if V1 registry key specified
                version = (string)Registry.GetValue(FabricInstalledRegPathDepreciated, FabricV1InstalledVersionRegKey, null);
            }

            return version;
        }

        protected override string GetTenantId()
        {
            const string TenantIDValueName = "WATenantID";
            string tenantIdKeyName = string.Format(CultureInfo.InvariantCulture, "{0}\\{1}", Registry.LocalMachine.Name, FabricConstants.FabricRegistryKeyPath);

            return (string)Registry.GetValue(tenantIdKeyName, TenantIDValueName, null);
        }
    }
}
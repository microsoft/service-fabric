// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.TokenValidationService
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.AzureActiveDirectory.Server;
    using System.Linq;

    internal sealed class AADSettings : IConfigStoreUpdateHandler
    {
        private const string SecuritySectionName = "Security";
        private const string TenantIdConfigName = "AADTenantId";
        private const string ClusterApplicationConfigName = "AADClusterApplication";
        private const string ClientApplicationConfigName = "AADClientApplication";
        private const string ClientRedirectUriConfigName = "AADClientRedirectUri";
        private const string LoginEndpointConfigName = "AADLoginEndpoint";
        private const string TokenIssuerFormatConfigName = "AADTokenIssuerFormat";
        private const string CertEndpointFormatConfigName = "AADCertEndpointFormat";
        private const string RoleClaimKeyConfigName = "AADRoleClaimKey";
        private const string AdminRoleClaimValueConfigName = "AADAdminRoleClaimValue";
        private const string UserRoleClaimValueConfigName = "AADUserRoleClaimValue";
        private const string SigningCertRolloverCheckIntervalConfigName = "AADSigningCertRolloverCheckInterval";

        private readonly Dictionary<string, string> configValues = new Dictionary<string, string>
        {
            { TenantIdConfigName, "" },
            { ClusterApplicationConfigName, "" },
            { ClientApplicationConfigName, "" },
            { ClientRedirectUriConfigName, "" },
            { LoginEndpointConfigName, "" },
            { TokenIssuerFormatConfigName, "" },
            { CertEndpointFormatConfigName, "" },
            { RoleClaimKeyConfigName, "" },
            { AdminRoleClaimValueConfigName, "" },
            { UserRoleClaimValueConfigName, "" },
            { SigningCertRolloverCheckIntervalConfigName, "" }
        };
        private IDictionary<string, string> defaultConfigValues = null;
        private readonly RwLock configValuesLock;

        private readonly NativeConfigStore nativeConfigStore;

        public string TenantId { get { return this.GetSetting(TenantIdConfigName); } }
        public string ClusterApplication { get { return this.GetSetting(ClusterApplicationConfigName); } }
        public string TokenIssuerFormat { get { return this.GetSetting(TokenIssuerFormatConfigName); } }
        public string CertEndpointFormat { get { return this.GetSetting(CertEndpointFormatConfigName); } }
        public string RoleClaimKey { get { return this.GetSetting(RoleClaimKeyConfigName); } }
        public string AdminRoleClaimValue { get { return this.GetSetting(AdminRoleClaimValueConfigName); } }
        public string UserRoleClaimValue { get { return this.GetSetting(UserRoleClaimValueConfigName); } }
        public int SigningCertRolloverCheckInterval { get { return this.GetIntSetting(SigningCertRolloverCheckIntervalConfigName, 3600); } }

        public AADSettings()
        {
            this.configValuesLock = new RwLock();
            this.nativeConfigStore = NativeConfigStore.FabricGetConfigStore(this);

            this.LoadGlobalDefaultSettings();
        }

        public string GetSetting(string configName)
        {
            using (var r = this.configValuesLock.AcquireReadLock())
            {
                return this.configValues[configName];
            }
        }

        public int GetIntSetting(string configName, int defaultValue)
        {
            int result = 0;
            if (!int.TryParse(GetSetting(configName), out result))
            {
                result = defaultValue;
            }

            return result;
        }

        // All AAD settings are currently static, but support dynamic settings
        // if they're ever added in the future.
        bool IConfigStoreUpdateHandler.OnUpdate(string section, string key)
        {
            if (section == SecuritySectionName)
            {
                using (var w = this.configValuesLock.AcquireWriteLock())
                {
                    this.LoadSetting_UnderLock(key);
                }
            }

            return true;
        }

        bool IConfigStoreUpdateHandler.CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        private void LoadGlobalDefaultSettings()
        {
            this.defaultConfigValues = System.Fabric.Security.AzureActiveDirectoryMetadata.GetDefaultAzureActiveDirectoryConfigurations();

            using (var w = this.configValuesLock.AcquireWriteLock())
            {
                foreach (var key in this.configValues.Keys.ToArray())
                {
                    TryLoadDefaultSetting_UnderLock(key);
                }
            }
        }

        private void LoadSetting_UnderLock(string key)
        {
            var value = this.nativeConfigStore.ReadUnencryptedString(SecuritySectionName, key);
            
            if (!string.IsNullOrEmpty(value))
            {
                this.configValues[key] = value;
            }
            else
            {
                TryLoadDefaultSetting_UnderLock(key);
            }
        }

        private void TryLoadDefaultSetting_UnderLock(string key)
        {
            string value = null;
            if (this.defaultConfigValues.TryGetValue(key, out value))
            {
                this.configValues[key] = value;
            }
        }
    }
}
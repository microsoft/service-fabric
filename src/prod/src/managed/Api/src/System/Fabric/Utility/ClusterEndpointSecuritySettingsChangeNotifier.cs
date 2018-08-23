// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal sealed class ClusterEndpointSecuritySettingsChangeNotifier : IConfigStoreUpdateHandler
    {
        private const string TraceType = "ClusterEndpointSecuritySettingsChangeNotifier";

        private readonly List<string> FabricNodeSectionSecurityKeyNames = new List<string>()
        {
            "ClusterX509StoreName",
            "ClusterX509FindType",
            "ClusterX509FindValue",
            "ClusterX509FindValueSecondary"
        };

        private readonly List<string> SecuritySectionSecurityKeyNames = new List<string>()
        {
            "ClusterCertThumbprints",
            "ClusterX509Names",
            "ClusterAllowedCommonNames",
            "ClusterCertIssuers",
            "ClusterIdentities"
        };

        private readonly List<string> ExtendedSecuritySectionNames = new List<string>()
        {
            "Security/ClusterX509Names",
            "Security/ServerX509Names",
            "Security/ClientX509Names",
            "Security/AdminClientX509Names",
            "Security/ClusterCertificateIssuerStore",
            "Security/ServerCertificateIssuerStore",
            "Security/ClientCertificateIssuerStore"
        };

        private readonly string endpoint;
        private readonly Action<string> updateReplicatorSettingsAction;
        private readonly NativeConfigStore configStore;

        public ClusterEndpointSecuritySettingsChangeNotifier(string endpoint, Action<string> updateReplicatorSettingsAction)
        {
            if (updateReplicatorSettingsAction == null)
            {
                throw new ArgumentNullException("updateReplicatorSettingsAction");
            }

            // null is allowed for endpoint
            this.endpoint = endpoint;
            this.updateReplicatorSettingsAction = updateReplicatorSettingsAction;
            this.configStore = NativeConfigStore.FabricGetConfigStore(this);
        }

        public bool OnUpdate(string section, string key)
        {
            if (this.ContainsRelevantSecuritySetting(section, key))
            {
                AppTrace.TraceSource.WriteInfo(TraceType, "Section '{0}', key '{1}' was updated and needs security settings needs update", section, key);

                this.updateReplicatorSettingsAction(endpoint);
            }

            AppTrace.TraceSource.WriteInfo(TraceType, "Exiting OnUpdate");
            return true;
        }

        public bool CheckUpdate(string sectionName, string keyName, string value, bool isEncrypted)
        {
            throw new InvalidOperationException(System.Fabric.Strings.StringResources.Error_InvalidOperation);
        }

        private bool ContainsRelevantSecuritySetting(string section, string key)
        {
            const string FabricNodeSection = "FabricNode";
            const string SecuritySection = "Security";

            if (section == FabricNodeSection)
            {
                if (FabricNodeSectionSecurityKeyNames.Contains(key))
                {
                    return true;
                }
            }
            else if (section == SecuritySection)
            {
                if (SecuritySectionSecurityKeyNames.Contains(key))
                {
                    return true;
                }
            }
            else if (ExtendedSecuritySectionNames.Contains(section))
            {
                return true;
            }

            return false;
        }
    }
}
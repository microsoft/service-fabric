// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Microsoft.ServiceFabric.DeploymentManager.Model;
    using Newtonsoft.Json;

    internal class StandAloneUserConfig : IUserConfig
    {
        [JsonConstructor]
        public StandAloneUserConfig()
        {
            this.AddonFeatures = new List<AddonFeature>();
        }

        public Security Security
        {
            get;
            set;
        }

        // don't use this property. keep it only for backward-compatibility purpose.
        public AzureActiveDirectory AzureActiveDirectory
        {
            get;
            set;
        }

        // don't use this property. keep it only for backward-compatibility purpose.
        public CertificateDescription Certificate
        {
            get;
            set;
        }

        // don't use this property. keep it only for backward-compatibility purpose.
        public List<ClientCertificateCommonName> ClientCertificateCommonNames
        {
            get;
            set;
        }

        // don't use this property. keep it only for backward-compatibility purpose.
        public List<ClientCertificateThumbprint> ClientCertificateThumbprints
        {
            get;
            set;
        }

        public DiagnosticsStorageAccountConfig DiagnosticsStorageAccountConfig
        {
            get;
            set;
        }

        public EncryptableDiagnosticStoreInformation DiagnosticsStoreInformation
        {
            get;
            set;
        }

        public List<VMResourceDescription> ExpectedVMResources
        {
            get;
            set;
        }

        public List<SettingsSectionDescription> FabricSettings
        {
            get;
            set;
        }

        public bool IsLinuxVM
        {
            get;
            set;
        }

        public bool IsVMSS
        {
            get;
            set;
        }

        public List<NodeTypeDescription> NodeTypes
        {
            get;
            set;
        }

        public IEnumerable<NodeTypeDescription> PrimaryNodeTypes
        {
            get;
            set;
        }

        public ReliabilityLevel ReliabilityLevel
        {
            get;
            set;
        }

        public int TotalNodeCount
        {
            get;
            set;
        }

        public int TotalPrimaryNodeTypeNodeCount
        {
            get;
            set;
        }

        public UserConfigVersion Version
        {
            get;
            set;
        }

        public string VMImage
        {
            get;
            set;
        }

        public string ClusterName
        {
            get;
            set;
        }

        public bool EnableTelemetry
        {
            get;
            set;
        }

        public bool AutoupgradeEnabled
        {
            get;
            set;
        }

        public bool IsScaleMin
        {
            get;
            set;
        }

        public List<AddonFeature> AddonFeatures
        {
            get;
            set;
        }

        public string CodeVersion
        {
            get;
            set;
        }

        public int GoalStateExpirationReminderInDays
        {
            get;
            set;
        }

        public StandAloneUserConfig Clone()
        {
            return (StandAloneUserConfig)this.MemberwiseClone();
        }

        public override bool Equals(object other)
        {
            var otherVersion = other as StandAloneUserConfig;
            if (otherVersion == null)
            {
                return false;
            }

            return base.Equals(other) && this.Version.Equals(otherVersion.Version);
        }

        public override int GetHashCode()
        {
            var hash = 27;
            hash = (13 * hash) + this.Version.GetHashCode();
            return hash;
        }
    }
}
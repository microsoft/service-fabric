// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using Newtonsoft.Json;
    using System.Collections.Generic;

    internal class MockupUserConfig : IUserConfig 
    {
        [JsonConstructor]
        public MockupUserConfig()
        {
            this.AddonFeatures = new List<AddonFeature>();
        }

        public UserConfigVersion Version { get; set; }

        public Security Security { get; set; }

        public List<VMResourceDescription> ExpectedVMResources { get; set; }

        public List<NodeTypeDescription> NodeTypes { get; set; }

        public List<SettingsSectionDescription> FabricSettings { get; set; }

        public DiagnosticsStorageAccountConfig DiagnosticsStorageAccountConfig { get; set; }

        public EncryptableDiagnosticStoreInformation DiagnosticsStoreInformation { get; set; }

        public string VMImage { get; set; }

        // ReSharper disable once InconsistentNaming - Backward compatibility.
        public bool IsVMSS { get; set; }

        public ReliabilityLevel ReliabilityLevel { get; set; }

        public int TotalNodeCount { get; set; }

        public IEnumerable<NodeTypeDescription> PrimaryNodeTypes { get; set; }

        public int TotalPrimaryNodeTypeNodeCount { get; set; }

        public bool IsLinuxVM { get; set; }

        public string ClusterName { get; set; }

        public bool EnableTelemetry { get; set; }

        public bool AutoupgradeEnabled { get; set; }

        public bool IsScaleMin { get; set; }

        public List<AddonFeature> AddonFeatures { get; set; }

        public string CodeVersion { get; set; }

        public bool AutoupgradeInstallEnabled { get; set; }

        public int GoalStateExpirationReminderInDays { get; set; }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Collections.Generic;

    internal interface IUserConfig
    {
        UserConfigVersion Version { get; set; }

        Security Security { get; set; }

        List<VMResourceDescription> ExpectedVMResources { get; set; }

        List<NodeTypeDescription> NodeTypes { get; set; }

        List<SettingsSectionDescription> FabricSettings { get; set; }

        DiagnosticsStorageAccountConfig DiagnosticsStorageAccountConfig { get; set; }

        EncryptableDiagnosticStoreInformation DiagnosticsStoreInformation { get; set; }

        string VMImage { get; set; }

        // ReSharper disable once InconsistentNaming - Backward compatibility.
        bool IsVMSS { get; set; }

        ReliabilityLevel ReliabilityLevel { get; set; }

        int TotalNodeCount { get; set; }

        IEnumerable<NodeTypeDescription> PrimaryNodeTypes { get; set; }

        int TotalPrimaryNodeTypeNodeCount { get; set; }

        bool IsLinuxVM { get; set; }

        string ClusterName { get; set; }

        bool EnableTelemetry { get; set; }
       
        bool AutoupgradeEnabled { get; set; }

        bool IsScaleMin { get; set; }

        List<AddonFeature> AddonFeatures { get; set; }

        string CodeVersion { get; set; }

        int GoalStateExpirationReminderInDays { get; set; }
    }
}
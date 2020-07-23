// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon.Test
{
    using System.Collections.Generic;
    using System.ComponentModel;

    internal class MockupProperty
    {
        public List<SettingsSectionDescription> FabricSettings
        {
            get;
            set;
        }

        public List<NodeTypeDescription> NodeTypes
        {
            get;
            set;
        }

        public Security Security
        {
            get;
            set;
        }

        [DefaultValue(true)]
        public bool EnableTelemetry
        {
            get;
            set;
        }

        public EncryptableDiagnosticStoreInformation DiagnosticsStore
        {
            get;
            set;
        }

        public DiagnosticsStorageAccountConfig DiagnosticsStorageAccountConfig 
        { 
            get; 
            set; 
        }

        public bool? FabricClusterAutoupgradeEnabled
        {
            get;
            set;
        }
    }
}
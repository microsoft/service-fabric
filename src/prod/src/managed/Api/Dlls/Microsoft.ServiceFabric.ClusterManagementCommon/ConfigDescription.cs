// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public enum UpgradePolicy
    {
        Dynamic,
        Static,
        NotAllowed,
        SingleChange
    }

    public class ConfigDescription
    {
        public string SectionName { get; set; }

        public string SettingName { get; set; }

        public string Type { get; set; }

        public string DefaultValue { get; set; }

        public UpgradePolicy UpgradePolicy { get; set; }

        public string SettingType { get; set; }
    }
}
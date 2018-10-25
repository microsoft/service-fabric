// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.UpgradeService
{
    public class ClusterUpgradeCommandParameter
    {
        public string ConfigFilePath { get; set; }

        public string ConfigVersion { get; set; }

        public string CodeFilePath { get; set; }

        public string CodeVersion { get; set; }

        public CommandProcessorClusterUpgradeDescription UpgradeDescription { get; set; }
    }
}
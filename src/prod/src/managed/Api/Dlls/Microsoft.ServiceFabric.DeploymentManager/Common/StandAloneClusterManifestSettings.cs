// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Common
{
    using System.Fabric.Management.ServiceModel;
    using Microsoft.ServiceFabric.ClusterManagementCommon;

    internal class StandAloneClusterManifestSettings : IClusterManifestSettings
    {
        public SettingsType CommonClusterSettings
        {
            get;
            set;
        }

        public string Version
        {
            get;
            set;
        }
    }
}
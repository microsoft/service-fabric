// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System.Fabric.Management.ServiceModel;

    public interface IClusterManifestSettings
    {
        string Version { get; set; }

        SettingsType CommonClusterSettings { get; set; }        
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    public interface IAdminConfig
    {
        AdminConfigVersion Version { get; set; }

        bool IsUserSet { get; set; }

        IClusterManifestSettings GetAdminFabricSettings();

        FabricSettingsMetadata GetFabricSettingsMetadata();
    }
}
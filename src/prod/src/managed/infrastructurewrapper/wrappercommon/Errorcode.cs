// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    internal enum ErrorCode
    {
        Success,
        Fail,
        Unexpected,

        MSVCR11InstallFailed,

        ClusterManifestInvalidLocation,
        ClusterManifestInvalidFormat,
        ClusterManifestInvalidInfrastructureElement,
        ClusterManifestInvalidNodeTypesElement,
        ClusterManifestInvalidNode,
        ClusterManifestDeploymentMismatch,

        ServicePackageInvalidPath,
        ServiceStartFailed,
        ServiceStopFailed,

        FabricUnzipFailed,
        FabricSchemaInvalidPath,
        FabricSchemaMultipleFilesFound,
        FabricDeploymentFailed,
        FabricInstallFailed,

        FabricPackageFileInvalidPath,
        FabricPackageFileMultipleFilesFound,

        SettingsInvalid,
        InvalidNodeList,
        NodeListGenerationFailed,
    }
}
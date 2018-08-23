// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    internal interface IApplicationValidator
    {
        // "isComposeDeployment" helps prevent a regular application provision using application type name with Prefix "Compose_".
        // "isSFVolumeDiskServiceEnabled" is a cluster level configuration and used to validate invalid usage of "UsesSFReplicatedStore"
        //  attribute in service policies.
        void Validate(ApplicationTypeContext applicationTypeInfo, bool isComposeDeployment, bool isSFVolumeDiskServiceEnabled);
        void Validate(ApplicationInstanceContext applicationInstanceInfo);
    }
}
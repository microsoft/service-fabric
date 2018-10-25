// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Security
{
    using System.Fabric.Interop;

    internal enum ResourceIdentifierKind
    {
        Invalid = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_INVALID,
        PathInFabricImageStore = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_PATH_IN_FABRIC_IMAGESTORE,
        ApplicationType = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION_TYPE,
        Application = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_APPLICATION,
        Service = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_SERVICE,
        Name = NativeTypes.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND.FABRIC_SECURITY_RESOURCE_IDENTIFIER_KIND_NAME
    }
}
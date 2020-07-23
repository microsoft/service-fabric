// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestoreEndPoints
{
    static class BackupRestoreEndPointsConstants
    {
        internal const string FabricUriHeaderKey = "X-Request-Metadata";
        internal const string Fabric404ErrorHeaderKey = "X-ServiceFabric";
        internal const string Fabric404ErrorHeaderValue = "ResourceNotFound";
        internal const string FabricXRoleHeaderKey = "X-Role";
    }

    internal enum FabricXRole
    {
        Invalid,
        None,
        User,
        Admin,
        All
    }
}
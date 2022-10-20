// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service.Models
{
    public static class GatewayResourceStatus
    {
        public const string Invalid = "Invalid";
        public const string Ready = "Ready";
        public const string Upgrading = "Upgrading";
        public const string Creating = "Creating";
        public const string Deleting = "Deleting";
        public const string Failed = "Failed";
    }
}

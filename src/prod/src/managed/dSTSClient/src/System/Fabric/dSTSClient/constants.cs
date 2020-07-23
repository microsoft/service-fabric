// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.dSTSClient
{
    internal static class Constants
    {
        public static readonly string dSTSClientLibraryName = "System.Fabric.dSTSClientImpl.dll";
        public static readonly string dSTSClientName = "System.Fabric.dSTSClient.dSTSClientImpl";
        public static readonly string GetSecurityToken = "GetSecurityToken";
        public static readonly string CreatedSTSClient = "CreatedSTSClient";

        public static readonly string[] dSTSKnownDlls = { "Microsoft.WindowsAzure.Security.Authentication.Contracts.dll", "Microsoft.WindowsAzure.Security.Authentication.dll", "Microsoft.WindowsAzure.Security.Authentication.Logging.dll", "Microsoft.IdentityModel.dll" };
    }
}

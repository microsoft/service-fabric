// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.ImageBuilder
{
    internal static class StringConstants
    {        
        public static readonly string DefaultSchemaPath = "ServiceFabricServiceModel.xsd";
        public static readonly string DoubleDot = "..";
        public static readonly string WindowsFabric = "WindowsFabric";        
        public static readonly string Msi = "msi";
        public static readonly string ComposeDeploymentTypePrefix = "Compose_";
        // 
        // Make sure the following value matched:
        // PartitionIdToken at /src/prod/src/managed/FabricCAS/ConfigHelper.cs
        // servicePartitionId at /src/prod/src/managed/Api/src/System/Fabric/Management/ImageBuilder/StringConstants.cs
        // 
        public static readonly string PartitionIdAlias = "@PartitionId393d4d7b-69cd-4561-9052-29f3685bf629@";

#if DotNetCoreClrLinux
        public static readonly string ServiceFabric = "servicefabric";
        public static readonly string Deb = "deb";
        public static readonly string Rpm = "rpm";
#endif
        public static readonly string Incoming = "Incoming";        
        public static readonly string FabricNamespace = "http://schemas.microsoft.com/2011/01/fabric";
        public static readonly string OpenNetworkConfig = "Open";
        public static readonly string IsolatedNetworkConfig = "Isolated";
        public static readonly string OtherNetworkConfig = "Other";
        public static readonly string SfpkgExtension = ".sfpkg";
        public static readonly string OpenNetworkName = "Open";
        public static readonly string NatNetworkName = "Other";
        public static readonly string TargetReplicaSetSizeKeyName = "TargetReplicaSetSize";
        public static readonly string FailoverManagerSectionName = "FailoverManager";
    }
}
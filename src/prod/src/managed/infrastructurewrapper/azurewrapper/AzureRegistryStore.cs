// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureUploaderCommon
{
    using System.IO;
    using Microsoft.Win32;

    internal static class AzureRegistryStore
    {
        private const string TenantIdRegistryKey = "WATenantID";
        private const string RoleInstanceIdRegistryKey = "WARoleInstanceID";
        private const string RoleNameRegistryKey = "WARoleName";

        private static readonly string KeyName = Path.Combine(Registry.LocalMachine.Name, System.Fabric.FabricConstants.FabricRegistryKeyPath);

        private static string deploymentId;
        private static string roleName;
        private static string roleInstanceId;

        // Writes Tenant ID to registry key HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Service Fabric\WATenantID
        // and HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows Fabric\WATenantID (for compatibility with <= 4.1 InfrastructureService).
        public static string DeploymentId
        {
            get
            {
                return AzureRegistryStore.deploymentId 
                    ?? (AzureRegistryStore.deploymentId = (string)Registry.GetValue(AzureRegistryStore.KeyName, AzureRegistryStore.TenantIdRegistryKey, null));
            }

            set
            {
                Registry.SetValue(
                    AzureRegistryStore.KeyName,
                    AzureRegistryStore.TenantIdRegistryKey,
                    value);
            }
        }

        // Writes RoleInstanceID to registry key HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Service Fabric\WARoleName (added post reg rename).
        public static string RoleName
        {
            get
            {
                return AzureRegistryStore.roleName 
                    ?? (AzureRegistryStore.roleName = (string)Registry.GetValue(AzureRegistryStore.KeyName, AzureRegistryStore.RoleNameRegistryKey, null));
            }

            set
            {
                Registry.SetValue(
                    AzureRegistryStore.KeyName,
                    AzureRegistryStore.RoleNameRegistryKey,
                    value);
            }
        }

        // Writes RoleInstanceID to registry key HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Service Fabric\WARoleInstanceID (added post reg rename).
        public static string RoleInstanceId
        {
            get
            {
                return AzureRegistryStore.roleInstanceId 
                    ?? (AzureRegistryStore.roleInstanceId = (string)Registry.GetValue(AzureRegistryStore.KeyName, AzureRegistryStore.RoleInstanceIdRegistryKey, null));
            }

            set
            {
                Registry.SetValue(
                    AzureRegistryStore.KeyName,
                    AzureRegistryStore.RoleInstanceIdRegistryKey,
                    value);
            }
        }
    }
}
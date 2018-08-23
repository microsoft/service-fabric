// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace AzureUploaderCommon
{
#if DotNetCoreClrLinux || DotNetCoreClrIOT
    using System;
    using System.Fabric.Common;
    using System.IO;
    using System.Xml;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    internal static class AzureFileStore
    {
        public const string azureDCAInfoFileName = @"/var/lib/waagent/ServiceFabricDcaInfo";
        private const string azureHostingEnvironmentFileName = @"/var/lib/waagent/HostingEnvironmentConfig.xml";
        private const string TenantIdKey = "ID";
        private const string RoleInstanceIdKey = "UD";
        private const string RoleNameKey = "FD";

        private static string deploymentId;
        private static string roleName;
        private static string roleInstanceId;
        
        public static string DeploymentId
        {
            get
            {
                if (string.IsNullOrEmpty(AzureFileStore.deploymentId))
                {
                    Initialize();
                }

                return AzureFileStore.deploymentId;
            }
        }
        
        public static string RoleName
        {
            get
            {
                if (string.IsNullOrEmpty(AzureFileStore.roleName))
                {
                    Initialize();
                }

                return AzureFileStore.roleName;
            }
        }
        
        public static string RoleInstanceId
        {
            get
            {
                if (string.IsNullOrEmpty(AzureFileStore.roleInstanceId))
                {
                    Initialize();
                }

                return AzureFileStore.roleInstanceId;
            }
        }

        public static void Initialize()
        {
            if (!File.Exists(azureHostingEnvironmentFileName))
            {
                // Set these values for single box cluster
                deploymentId = Guid.NewGuid().ToString();
                roleName = Guid.NewGuid().ToString();
                roleInstanceId = Helpers.GetMachine();

                return;
            }

            try
            {
                XmlDocument xmlDoc = new XmlDocument();

                xmlDoc.Load(new FileStream(azureHostingEnvironmentFileName, FileMode.Open));

                deploymentId = xmlDoc.GetElementsByTagName("Deployment").Item(0).Attributes.GetNamedItem("name").Value.Replace("-", "");
                roleName = xmlDoc.GetElementsByTagName("Role").Item(0).Attributes.GetNamedItem("name").Value;

                using (StreamReader file = File.OpenText(azureDCAInfoFileName))
                {
                    using (JsonTextReader reader = new JsonTextReader(file))
                    {
                        JToken jsonToken = JToken.ReadFrom(reader);
                        roleInstanceId = jsonToken["NodeInstanceInfo"]["ID"].ToString();
                    }
                }
            }
            catch
            {
                deploymentId = String.Empty;
                roleName = String.Empty;
                roleInstanceId = String.Empty;
            }
        }
    }
#endif
}
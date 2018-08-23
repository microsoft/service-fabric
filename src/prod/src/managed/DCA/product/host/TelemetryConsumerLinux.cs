// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Fabric.Dca;
    using System.IO;
    using System.Reflection;
    using System.Xml;

    internal class TelemetryConsumerLinux : TelemetryConsumerBase
    {
        private const string TenantIdFile = @"/var/lib/waagent/HostingEnvironmentConfig.xml";
        private const string ServiceFabricVersionFileName = "ClusterVersion";

        public TelemetryConsumerLinux(string applicationInstanceId)
            : base(applicationInstanceId)
        {
        }

        protected override string GetServiceFabricVersion()
        {
            string version = "undefined";
            string versionFilePath = string.Empty;

            try
            {
                versionFilePath = Path.Combine(Path.GetDirectoryName(Assembly.GetEntryAssembly().Location), TelemetryConsumerLinux.ServiceFabricVersionFileName);
                version = File.ReadAllText(versionFilePath);
            }
            catch(Exception e)
            {
                Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        string.Format("Failed to read Service Fabric version from file: {0}", versionFilePath));
            }

            return version;
        }

        protected override string GetTenantId()
        {
            if (!File.Exists(TelemetryConsumerLinux.TenantIdFile))
            {
                return null;
            }

            string tenantId;

            try
            {
                XmlDocument xmlDoc = new XmlDocument();

                xmlDoc.Load(new FileStream(TelemetryConsumerLinux.TenantIdFile, FileMode.Open));

                tenantId = xmlDoc.GetElementsByTagName("Deployment").Item(0).Attributes.GetNamedItem("name").Value;
            }
            catch (Exception e)
            {
                Utility.TraceSource.WriteExceptionAsError(
                        TraceType,
                        e,
                        "Failed to read TenantId from file");
                tenantId = null;
            }

            return tenantId;
        }
    }
}
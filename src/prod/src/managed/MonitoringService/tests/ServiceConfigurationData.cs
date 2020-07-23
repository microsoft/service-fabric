// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Tests
{
    using System;
    using Internal;

    /// <summary>
    /// Contains configuration values to setup IServiceConfiguration mock.
    /// </summary>
    internal class ServiceConfigurationData : IInternalServiceConfiguration
    {
        public ServiceConfigurationData()
        {
            this.Refresh();
        }

        public string ApplicationsThatReportDeployedApplicationHealth { get; set; }

        public string ApplicationsThatReportHealthEvents { get; set; }

        public string ApplicationsThatReportPartitionHealth { get; set; }

        public string ApplicationsThatReportReplicaHealth { get; set; }

        public string ApplicationsThatReportServiceHealth { get; set; }

        public string ApplicationsThatReportServicePackageHealth { get; set; }

        public string ClusterName { get; set; }

        public string DataCenter { get; set; }

        public TimeSpan HealthDataReadInterval { get; set; }

        public TimeSpan HealthDataReadTimeout { get; set; }

        public TimeSpan HealthQueryTimeoutInSeconds { get; set; }

        public string MdmAccountName { get; set; }

        public string MdmNamespace { get; set; }

        public string ReportDeployedApplicationHealth { get; set; }

        public string ReportHealthEvents { get; set; }

        public string ReportPartitionHealth { get; set; }

        public string ReportReplicaHealth { get; set; }

        public string ReportServiceHealth { get; set; }

        public string ReportServicePackageHealth { get; set; }

        public void Refresh()
        {
            // set default values
            this.ClusterName = "unit-test-cluster";
            this.HealthDataReadInterval = TimeSpan.FromMinutes(1);
            this.HealthDataReadTimeout = TimeSpan.FromSeconds(60);
            this.HealthQueryTimeoutInSeconds = TimeSpan.FromSeconds(1);
            this.ReportServiceHealth = "Always";
            this.ApplicationsThatReportServiceHealth = "fabric:/System,fabric:/Monitoring";
            this.ReportPartitionHealth = "Always";
            this.ApplicationsThatReportPartitionHealth = "fabric:/System,fabric:/Monitoring";
            this.ReportReplicaHealth = "Always";
            this.ApplicationsThatReportReplicaHealth = "fabric:/System,fabric:/Monitoring";
            this.ReportDeployedApplicationHealth = "Never";
            this.ApplicationsThatReportDeployedApplicationHealth = string.Empty;
            this.ReportServicePackageHealth = "Never";
            this.ApplicationsThatReportServicePackageHealth = string.Empty;

            this.ReportHealthEvents = "Always";
            this.ApplicationsThatReportHealthEvents = "All";

            this.MdmAccountName = "SFGMTest";
            this.MdmNamespace = "SFGMTestNs";
            this.DataCenter = "OneBox";
        }
    }
}
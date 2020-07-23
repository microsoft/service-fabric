// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Interfaces
{
    using System;

    internal interface IServiceConfiguration
    {
        TimeSpan HealthDataReadInterval { get; }

        TimeSpan HealthDataReadTimeout { get; }

        TimeSpan HealthQueryTimeoutInSeconds { get; }

        string ClusterName { get; }

        string ReportServiceHealth { get; }

        string ReportPartitionHealth { get; }

        string ReportReplicaHealth { get; }

        string ReportDeployedApplicationHealth { get; }

        string ReportServicePackageHealth { get; }

        string ReportHealthEvents { get; }

        string ApplicationsThatReportServiceHealth { get; }

        string ApplicationsThatReportPartitionHealth { get; }

        string ApplicationsThatReportReplicaHealth { get; }

        string ApplicationsThatReportDeployedApplicationHealth { get; }

        string ApplicationsThatReportServicePackageHealth { get; }

        string ApplicationsThatReportHealthEvents { get; }

        void Refresh();
    }
}
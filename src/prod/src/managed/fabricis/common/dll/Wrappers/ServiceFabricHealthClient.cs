// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Health;

    internal class ServiceFabricHealthClient : IHealthClient
    {
        private readonly FabricClient.HealthClient healthClient = new FabricClient().HealthManager;

        public void ReportHealth(HealthReport healthReport)
        {
            healthReport.Validate("healthReport");
            healthClient.ReportHealth(healthReport);
        }
    }
}
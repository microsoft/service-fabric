// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Common;

    internal class ManagementClientFactory : IManagementClientFactory
    {
        private static readonly TraceType TraceType = new TraceType("ManagementClientFactory");

        public IManagementClient Create()
        {
            IManagementClient managementClient = new WindowsAzureManagementClient();
            Trace.WriteInfo(TraceType, "Created new management client of type: {0}", typeof(WindowsAzureManagementClient).Name);

            return managementClient;
        }
    }
}
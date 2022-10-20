// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.

#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Description;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class CreateServiceFromTemplateRequest : FabricRequest
    {
        public CreateServiceFromTemplateRequest(
            IFabricClient fabricClient, 
            Uri applicationName, 
            Uri serviceName,
            string serviceDnsName, 
            string serviceTypeName,
            ServicePackageActivationMode servicePackageActivationMode,
            byte[] initializationData, 
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(applicationName, "applicationName");
            ThrowIf.Null(serviceName, "serviceName");
            ThrowIf.NullOrEmpty(serviceTypeName, "serviceTypeName");

            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
            this.ServiceDnsName = serviceDnsName;
            this.ServiceTypeName = serviceTypeName;
            this.ServicePackageActivationMode = servicePackageActivationMode;
            this.InitializationData = initializationData;

            this.ConfigureErrorCodes();
        }

        public Uri ApplicationName
        {
            get;
            private set;
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        public string ServiceTypeName
        {
            get;
            private set;
        }

        public string ServiceDnsName
        {
            get;
            private set;
        }

        public ServicePackageActivationMode ServicePackageActivationMode
        {
            get;
            private set;
        }

        public byte[] InitializationData
        {
            get;
            private set;
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult =
                await this.FabricClient.CreateServiceFromTemplateAsync(
                    this.ApplicationName, 
                    this.ServiceName, 
                    this.ServiceDnsName,
                    this.ServiceTypeName,
                    this.ServicePackageActivationMode,
                    this.InitializationData, 
                    this.Timeout, cancellationToken);
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Create service from template (ApplicationName: {0} ServiceName: {1} ServiceTypeName: {2} ServicePackageActivationMode: {3} InitializationData: {4} with timeout {5})",
                this.ApplicationName,
                this.ServiceName,
                this.ServiceTypeName,
                this.ServicePackageActivationMode,
                this.InitializationData,
                this.Timeout);
        }

        private void ConfigureErrorCodes()
        {
            this.SucceedErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_SERVICE_ALREADY_EXISTS);
        }
    }
}


#pragma warning restore 1591
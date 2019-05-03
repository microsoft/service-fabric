// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests.Internal
{
    using System;
    using System.Globalization;
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;

    public class RestartCodePackageRequest : FabricRequest
    {
        public RestartCodePackageRequest(IFabricClient fabricClient,
            string nodeName,
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            long codePackageInstanceId,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");
            ThrowIf.Null(applicationName, "applicationName");
            ThrowIf.Null(serviceManifestName, "serviceManifestName");
            ThrowIf.Null(codePackageName, "codePackageName");
            ThrowIf.Null(servicePackageActivationId, "servicePackageActivationId");

            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestName = serviceManifestName;
            this.CodePackageName = codePackageName;
            this.CodePackageInstanceId = codePackageInstanceId;
            this.ServicePackageActivationId = servicePackageActivationId;
        }

        public Uri ApplicationName  { get; private set; }

        public string ServiceManifestName { get; private set; }

        public string ServicePackageActivationId { get; private set; }

        public string CodePackageName  { get; private set; }

        public string NodeName { get; private set; }

        public long CodePackageInstanceId { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Restart CodePackage with node {0}, code package: {1} timeout {2}", this.NodeName, this.CodePackageName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.RestartDeployedCodePackageNativeAsync(
                this.NodeName,
                this.ApplicationName,
                this.ServiceManifestName,
                this.ServicePackageActivationId,
                this.CodePackageName,
                this.CodePackageInstanceId, 
                this.Timeout, 
                cancellationToken);
        }
    }
}


#pragma warning restore 1591
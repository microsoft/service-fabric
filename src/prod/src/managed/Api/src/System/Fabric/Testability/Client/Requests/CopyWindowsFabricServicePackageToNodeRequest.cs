// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class CopyWindowsFabricServicePackageToNodeRequest : FabricRequest
    {
        public CopyWindowsFabricServicePackageToNodeRequest(IFabricClient fabricClient, string serviceManifestName, string applicationTypeName, string applicationTypeVersion, string nodeName, PackageSharingPolicyDescription[] policies, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.NullOrEmpty(serviceManifestName, "serviceManifestName");
            ThrowIf.NullOrEmpty(applicationTypeName, "applicationTypeName");
            ThrowIf.NullOrEmpty(applicationTypeVersion, "applicationTypeVersion");
            ThrowIf.NullOrEmpty(nodeName, "nodeName");

            this.ServiceManifestName = serviceManifestName;
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.NodeName = nodeName;
            this.Policies = policies;
        }

        public string ServiceManifestName
        {
            get;
            private set;
        }

        public string ApplicationTypeName
        {
            get;
            private set;
        }

        public string ApplicationTypeVersion
        {
            get;
            private set;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public PackageSharingPolicyDescription[] Policies
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Copy service package to node request (ServiceManifestName: {0}, ApplicationTypeName: {1}, ApplicationTypeVersion: {2}, NodeName: {3}, Timeout {4})", this.ServiceManifestName, this.ApplicationTypeName, this.ApplicationTypeVersion, this.NodeName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CopyWindowsFabricServicePackageToNodeAsync(this.ServiceManifestName, this.ApplicationTypeName, this.ApplicationTypeVersion, this.NodeName, this.Policies, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591
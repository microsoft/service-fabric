// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class NewNodeConfigurationRequest : FabricRequest
    {
        public NewNodeConfigurationRequest(IFabricClient fabricClient, string clusterManifestPath, string computerName, string userName, string password)
            : base(fabricClient, TimeSpan.MaxValue/*API does not take a timeout*/)
        {
            ThrowIf.NullOrEmpty(clusterManifestPath, "clusterManifestPath");

            this.ClusterManifestPath = clusterManifestPath;
            this.ComputerName = computerName;
            this.UserName = userName;
            this.Password = password;
        }

        public string ClusterManifestPath
        {
            get;
            set;
        }

        public string ComputerName
        {
            get;
            set;
        }

        public string UserName
        {
            get;
            set;
        }

        public string Password
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "NewClusterConfigurationRequest clusterManifestPath: {0} computerName: {1} userName: {2} password : {3} timeout: {4})", this.ClusterManifestPath, this.ComputerName, this.UserName, this.Password, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.NewNodeConfigurationAsync(this.ClusterManifestPath, this.ComputerName, this.UserName, this.Password, cancellationToken);
        }
    }
}


#pragma warning restore 1591
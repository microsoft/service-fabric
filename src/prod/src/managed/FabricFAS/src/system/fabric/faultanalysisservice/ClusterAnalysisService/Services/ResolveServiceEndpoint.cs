// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FaultAnalysis.Service.ClusterAnalysis.Services
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using global::ClusterAnalysis.Common;
    using global::ClusterAnalysis.Common.Log;
    using Microsoft.ServiceFabric.Services.Client;

    using Newtonsoft.Json.Linq;

    /// <summary>
    /// </summary>
    internal class ResolveServiceEndpoint : IResolveServiceEndpoint
    {
        private ServicePartitionResolver partitionResolver;

        private ILogger logger = LocalDiskLogger.LogProvider.CreateLoggerInstance("ResolveServiceEndpoint.txt");

        /// <summary>
        /// Create an instance of <see cref="ResolveServiceEndpoint"/>
        /// </summary>
        /// <param name="resolver"></param>
        public ResolveServiceEndpoint(ServicePartitionResolver resolver)
        {
            this.partitionResolver = resolver;
        }

        /// <inheritdoc />
        public async Task<Uri> GetServiceEndpointAsync(Uri serviceUri, string hostNodeId, string resourceToAccess, CancellationToken token)
        {
            var resolvedPartitions = await this.partitionResolver.ResolveAsync(serviceUri, ServicePartitionKey.Singleton, token).ConfigureAwait(false);

            var endpoints = resolvedPartitions.Endpoints.ToList();
            this.logger.LogMessage("GetEndpointOfServiceHostedOnThisNodeAsync:: Total Endpoints Count '{0}'", endpoints.Count);

            foreach (var singleEndpoint in endpoints)
            {
                var address = (string)JObject.Parse(singleEndpoint.Address)["Endpoints"].First;
                if (!address.Contains(hostNodeId))
                {
                    continue;
                }

                address = address.EndsWith("/") ? address.Remove(address.Length - 1) : address;
                return new Uri(string.Format(CultureInfo.InvariantCulture, "{0}/{1}", address, resourceToAccess));
            }

            this.logger.LogMessage("GetEndpointOfServiceHostedOnThisNodeAsync:: No Match Found for HostId '{0}'", hostNodeId);
            throw new Exception(string.Format(CultureInfo.InvariantCulture, "No Endpoint Found for Host '{0}", hostNodeId));
        }
    }
}
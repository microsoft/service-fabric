// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.GatewayResourceManager.Service
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.GatewayResourceManager.Service.Models;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;
    using Newtonsoft.Json;

    internal sealed class GatewayResourceManagerImpl : IGatewayResourceManager
    {
        private const string TraceType = "GatewayResourceManagerImpl";
        private readonly IReliableStateManager reliableStateManager;
        private readonly ServiceContext serviceContext;

        public GatewayResourceManagerImpl(IReliableStateManager reliableStateManager, ServiceContext serviceContext)
        {
            this.reliableStateManager = reliableStateManager;
            this.serviceContext = serviceContext;
        }

        public async Task<string> UpdateOrCreateAsync(string resourceDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var resourceDescriptionObject = JsonConvert.DeserializeObject<GatewayResourceDescription>(resourceDescription);
            var requestHandler = new Manager(resourceDescriptionObject.Name, this.reliableStateManager, this.serviceContext);
            var result = await requestHandler.UpgradeOrAddResource(resourceDescriptionObject, timeout, cancellationToken);
            return JsonConvert.SerializeObject(result.Item2);
        }

        public async Task<List<string>> QueryAsync(string queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var requestHandler = new Manager(queryDescription, this.reliableStateManager, this.serviceContext);
            Tuple<int, List<GatewayResourceDescription>> result;
            if (string.IsNullOrEmpty(queryDescription))
            {
                result = await requestHandler.GetConfigs(timeout, cancellationToken);
            }
            else
            {
                result = await requestHandler.GetConfigForGateway(timeout, cancellationToken);
            }

            var list = new List<string>();
            if (result.Item2 == null)
            {
                return list;
            }

            foreach (var entry in result.Item2)
            {
                list.Add(JsonConvert.SerializeObject(entry));
            }

            return list;
        }

        public async Task DeleteAsync(string resourceName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            var requestHandler = new Manager(resourceName, this.reliableStateManager, this.serviceContext);
            await requestHandler.DeleteResource(timeout, cancellationToken);
            return;
        }
   }
}
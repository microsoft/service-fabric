// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    internal class PropertyManagerWrapper : IPropertyManagerWrapper
    {
        private readonly FabricClient fabricClient = new FabricClient();

        public Task CreateNameAsync(Uri name)
        {
            return fabricClient.PropertyManager.CreateNameAsync(name);
        }

        public Task<bool> NameExistsAsync(Uri name)
        {
            return fabricClient.PropertyManager.NameExistsAsync(name);
        }

        public async Task<IPropertyBatchResultWrapper> SubmitPropertyBatchAsync(Uri parentName, ICollection<PropertyBatchOperation> operations)
        {
            PropertyBatchResult result =
                await
                    fabricClient.PropertyManager.SubmitPropertyBatchAsync(parentName, operations).ConfigureAwait(false);

            return new PropertyBatchResultWrapper(result);
        }
    }    
}
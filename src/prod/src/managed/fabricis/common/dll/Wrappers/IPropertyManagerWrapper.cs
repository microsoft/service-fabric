// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// Wrapper interface for <see cref="FabricClient.PropertyManager"/>
    /// </summary>
    internal interface IPropertyManagerWrapper
    {
        Task CreateNameAsync(Uri name);

        Task<bool> NameExistsAsync(Uri name);

        Task<IPropertyBatchResultWrapper> SubmitPropertyBatchAsync(Uri parentName, ICollection<PropertyBatchOperation> operations);
    }
}
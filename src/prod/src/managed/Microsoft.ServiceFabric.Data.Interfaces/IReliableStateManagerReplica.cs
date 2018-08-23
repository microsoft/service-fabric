// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// Defines replica of a reliable state provider. 
    /// </summary>
    public interface IReliableStateManagerReplica : IStateProviderReplica, IReliableStateManager
    {
    }
}
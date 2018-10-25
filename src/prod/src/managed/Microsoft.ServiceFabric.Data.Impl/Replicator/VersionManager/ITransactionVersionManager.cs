// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Threading.Tasks;

    internal interface ITransactionVersionManager
    {
        Task<long> RegisterAsync();

        void UnRegister(long visibilityVersionNumber);
    }
}
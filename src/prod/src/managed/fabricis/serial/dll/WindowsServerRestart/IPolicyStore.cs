// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Threading;
    using System.Threading.Tasks;

    internal interface IPolicyStore
    {
        Task<bool> KeyPresentAsync(string key, TimeSpan timeout, CancellationToken token);

        Task<byte[]> GetDataAsync(string key, TimeSpan timeout, CancellationToken token);

        Task AddOrUpdate(string key, byte[] data, TimeSpan timeout, CancellationToken token);
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using Microsoft.ServiceFabric.Data;
    using System.Threading.Tasks;

    /// <summary>
    /// Interface for supporting sweep operation.
    /// </summary>
    internal interface IStoreSweepProvider
    {
        int SweepThreshold { get; }
      void TryStartSweep();
    }
}
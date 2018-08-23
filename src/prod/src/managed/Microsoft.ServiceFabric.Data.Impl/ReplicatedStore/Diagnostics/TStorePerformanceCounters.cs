// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Store;

    class TStorePerformanceCounters : IFabricPerformanceCountersDefinition
    {
        private static readonly
            Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>>
            CounterSets =
                new Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>>()
                {
                    {
                        new FabricPerformanceCounterSetDefinition(
                            DifferentialStoreConstants.PerformanceCounters.Name,
                            DifferentialStoreConstants.PerformanceCounters.Description,
                            FabricPerformanceCounterCategoryType.MultiInstance,
                            Guid.Parse("86AA2999-640C-45C0-A3E0-F85C3BB20005"),
                            "ServiceFabricTStore"),
                        new[]
                        {
                            new FabricPerformanceCounterDefinition(
                                1,
                                DifferentialStoreConstants.PerformanceCounters.ItemCountName,
                                DifferentialStoreConstants.PerformanceCounters.ItemCountDescription,
                                FabricPerformanceCounterType.NumberOfItems64,
                                DifferentialStoreConstants.PerformanceCounters.ItemCountSymbol),
                            new FabricPerformanceCounterDefinition(
                                2,
                                DifferentialStoreConstants.PerformanceCounters.DiskSizeName,
                                DifferentialStoreConstants.PerformanceCounters.DiskSizeDescription,
                                FabricPerformanceCounterType.NumberOfItems64,
                                DifferentialStoreConstants.PerformanceCounters.DiskSizeSymbol),
                            new FabricPerformanceCounterDefinition(
                                3,
                                DifferentialStoreConstants.PerformanceCounters.CheckpointFileWriteBytesPerSecName,
                                DifferentialStoreConstants.PerformanceCounters.CheckpointFileWriteBytesPerSecDescription,
                                FabricPerformanceCounterType.NumberOfItems64,
                                DifferentialStoreConstants.PerformanceCounters.CheckpointFileWriteBytesPerSecSymbol),
                            new FabricPerformanceCounterDefinition(
                                4,
                                DifferentialStoreConstants.PerformanceCounters.CopyDiskTransferBytesPerSecName,
                                DifferentialStoreConstants.PerformanceCounters.CopyDiskTransferBytesPerSecDescription,
                                FabricPerformanceCounterType.NumberOfItems64,
                                DifferentialStoreConstants.PerformanceCounters.CopyDiskTransferBytesPerSecSymbol),
                        }
                    }
                };
        
        public Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>> GetCounterSets()
        {
            return CounterSets;
        }
    }
}

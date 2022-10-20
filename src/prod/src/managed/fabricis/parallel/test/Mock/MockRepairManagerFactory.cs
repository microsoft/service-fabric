// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    internal sealed class MockRepairManagerFactory : IRepairManagerFactory
    {
        private IRepairManager repairManager;

        public IRepairManager Create()
        {
            return repairManager ?? (repairManager = new MockRepairManager());
        }
    }
}
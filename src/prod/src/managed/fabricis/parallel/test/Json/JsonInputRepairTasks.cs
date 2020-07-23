// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel.Test
{
    using Collections.Generic;

    internal sealed class JsonInputRepairTasks
    {
        public List<MockRepairTask> RepairTasks { get; set; }

        public override string ToString()
        {
            var text = "Items: {0}".ToString(RepairTasks != null ? RepairTasks.Count.ToString() : "<null>");
            return text;
        }
    }
}
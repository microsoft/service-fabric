// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal class FabricPerformanceCounterSetDefinition
    {
        public string Name { get; private set; }
        public string Description { get; private set; }
        public FabricPerformanceCounterCategoryType CategoryType { get; private set; }
        public Guid Guid { get; private set; }
        public string Symbol { get; private set; }

        public FabricPerformanceCounterSetDefinition(
            string name,
            string description,
            FabricPerformanceCounterCategoryType categoryType,
            Guid guid,
            string symbol)
        {
            Name = name;
            Description = description;
            CategoryType = categoryType;
            Guid = guid;
            Symbol = symbol;
        }
    }
}
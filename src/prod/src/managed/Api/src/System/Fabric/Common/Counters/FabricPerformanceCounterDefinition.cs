// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Collections.Generic;

    internal class FabricPerformanceCounterDefinition
    {
            public int Id { get; private set; }
            public int BaseId { get; private set; }
            public string Name { get; private set; }
            public string Description { get; private set; }
            public FabricPerformanceCounterType CounterType { get; private set; }
            public string Symbol { get; private set; }
            public IEnumerable<string> Attributes { get; private set; }

            public FabricPerformanceCounterDefinition(
                int id,
                int baseId,
                string name,
                string description,
                FabricPerformanceCounterType counterType,
                string symbol,
                IEnumerable<string> attributes)
            {
                Id = id;
                BaseId = baseId;
                Name = name;
                Description = description;
                CounterType = counterType;
                Symbol = symbol;
                Attributes = attributes;
            }

            public FabricPerformanceCounterDefinition(
                int id,
                string name,
                string description,
                FabricPerformanceCounterType counterType,
                string symbol) :
                this(id, Int32.MinValue, name, description, counterType, symbol, new List<string>())
            {
            }

            public FabricPerformanceCounterDefinition(
                int id,
                int baseId,
                string name,
                string description,
                FabricPerformanceCounterType counterType,
                string symbol) :
                this(id, baseId, name, description, counterType, symbol, new List<string>())
            {
            }

            public FabricPerformanceCounterDefinition(
                int id,
                string name,
                string description,
                FabricPerformanceCounterType counterType,
                string symbol,
                IEnumerable<string> attributes) :
                this(id, Int32.MinValue, name, description, counterType, symbol, attributes)
            {
            }
    }
}
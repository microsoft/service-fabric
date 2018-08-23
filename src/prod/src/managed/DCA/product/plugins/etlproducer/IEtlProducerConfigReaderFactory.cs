// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System.Fabric.Common.Tracing;

    internal interface IEtlProducerConfigReaderFactory
    {
        IEtlProducerConfigReader CreateEtlProducerConfigReader(
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId);
    }
}
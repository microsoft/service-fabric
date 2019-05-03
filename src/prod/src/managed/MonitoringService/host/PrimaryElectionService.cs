// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricMonSvc
{
    using System.Fabric;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.Services.Runtime;

    /// <summary>
    /// Base class for a service that behaves as stateful service to get leader election 
    /// but does not actually have any state to replicate.
    /// </summary>
    internal class PrimaryElectionService : StatefulServiceBase
    {
        protected PrimaryElectionService(StatefulServiceContext serviceContext, TraceWriterWrapper traceWriter)
            : base(serviceContext, new NullStateProviderReplica(traceWriter))
        {
        }
    }
}
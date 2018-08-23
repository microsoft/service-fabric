// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric.Common;

    internal class Tracer : ITracer
    {
        public Tracer(Guid partitionId, long replicaId)
        {
            this.Type = partitionId.ToString() + '.' + replicaId;
        }

        public string Type { get; private set; }

        public void WriteError(string type, string format, params object[] args)
        {
            var qualifiedType = type + this.Type;
            AppTrace.TraceSource.WriteError(qualifiedType, format, args);
        }

        public void WriteInfo(string type, string format, params object[] args)
        {
            var qualifiedType = type + this.Type;
            AppTrace.TraceSource.WriteInfo(qualifiedType, format, args);
        }

        public void WriteNoise(string type, string format, params object[] args)
        {
            var qualifiedType = type + this.Type;
            AppTrace.TraceSource.WriteNoise(qualifiedType, format, args);
        }

        public void WriteWarning(string type, string format, params object[] args)
        {
            var qualifiedType = type + this.Type;
            AppTrace.TraceSource.WriteWarning(qualifiedType, format, args);
        }
    }
}
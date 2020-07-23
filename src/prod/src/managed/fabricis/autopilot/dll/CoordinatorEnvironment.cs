// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    internal sealed class CoordinatorEnvironment
    {
        public CoordinatorEnvironment(string serviceName, IConfigSection configSection, string baseTraceId)
        {
            this.ServiceName = serviceName.Validate("serviceName");
            this.Config = configSection.Validate("configSection");
            this.DefaultTraceType = new TraceType(Constants.TraceTypeName, baseTraceId);
        }

        public string ServiceName { get; private set; }

        public IConfigSection Config { get; private set; }

        public TraceType DefaultTraceType { get; private set; }

        public TraceType CreateTraceType(string typeNameSuffix)
        {
            return new TraceType(
                this.DefaultTraceType.Name + typeNameSuffix,
                this.DefaultTraceType.Id);
        }
    }
}
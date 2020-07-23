// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;
    using System.Fabric.Common;

    internal sealed class CoordinatorFactoryArgs
    {
        public Uri ServiceName { get; set; }
        public IConfigSection ConfigSection { get; set; }
        public string ConfigSectionName
        {
            get { return this.ConfigSection.Name; }
        }
        public Guid PartitionId { get; set; }
        public long ReplicaId { get; set; }
        public IInfrastructureAgentWrapper Agent { get; set; }
    }
}
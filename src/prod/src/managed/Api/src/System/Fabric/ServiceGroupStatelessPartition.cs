// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal class ServiceGroupStatelessPartition : StatelessServicePartition, IServiceGroupPartition
    {
        private readonly NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition;

        internal ServiceGroupStatelessPartition(StatelessServiceBroker statelessServiceBroker, NativeRuntime.IFabricStatelessServicePartition nativeStatelessPartition, NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition)
            : base(statelessServiceBroker, nativeStatelessPartition)
        {
            Requires.Argument("nativeServiceGroupPartition", nativeServiceGroupPartition).NotNull();
            Requires.Argument("nativeStatelessPartition", nativeStatelessPartition).NotNull();

            this.nativeServiceGroupPartition = nativeServiceGroupPartition;
        }

        public T ResolveMember<T>(Uri name) where T : class
        {
            Requires.Argument("name", name).NotNull();

            return ServiceGroupPartitionHelper.ResolveMember<T>(this.nativeServiceGroupPartition, name);
        }
    }
}
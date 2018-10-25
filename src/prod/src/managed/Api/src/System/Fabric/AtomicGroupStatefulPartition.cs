// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    [SuppressMessage(FxCop.Category.Maintainability, FxCop.Rule.AvoidExcessiveClassCoupling, Justification = "By design.")]
    internal class AtomicGroupStatefulServicePartition : StatefulServicePartition, IServiceGroupPartition
    {
        private readonly NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition;

        internal AtomicGroupStatefulServicePartition(
            NativeRuntime.IFabricStatefulServicePartition nativeStatefulPartition,
            NativeRuntime.IFabricServiceGroupPartition nativeServiceGroupPartition,
            ServicePartitionInformation partitionInfo)
            : base(nativeStatefulPartition, partitionInfo)
        {
            Requires.Argument("nativeStatefulPartition", nativeStatefulPartition).NotNull();
            Requires.Argument("nativeServiceGroupPartition", nativeServiceGroupPartition).NotNull();
            Requires.Argument("partitionInfo", partitionInfo).NotNull();

            this.nativeServiceGroupPartition = nativeServiceGroupPartition;
        }

        public T ResolveMember<T>(Uri name) where T : class
        {
            Requires.Argument("name", name).NotNull();

            return ServiceGroupPartitionHelper.ResolveMember<T>(this.nativeServiceGroupPartition, name);
        }
    }
}
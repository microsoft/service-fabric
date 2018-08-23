// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ResourceGovernancePolicyDescription
    {
        internal ResourceGovernancePolicyDescription()
        {
        }

        internal string CodePackageRef { get; set; }

        internal UInt32 MemoryInMB { get; set; }

        internal UInt32 MemorySwapInMB { get; set; }

        internal UInt32 MemoryReservationInMB { get; set; }

        internal UInt32 CpuShares { get; set; }

        internal UInt32 CpuPercent { get; set; }

        internal UInt32 MaximumIOps { get; set; }

        internal UInt32 MaximumIOBytesps { get; set; }

        internal UInt32 BlockIOWeight { get; set; }

        internal string CpusetCpus { get; set; }

        internal UInt64 NanoCpus { get; set; }

        internal UInt32 CpuQuota { get; set; }

        internal UInt64 DiskQuotaInMB { get; set; }

        internal UInt64 KernelMemoryInMB { get; set; }

        internal UInt64 ShmSizeInMB { get; set; }

        internal static unsafe ResourceGovernancePolicyDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(
                nativePtr != IntPtr.Zero,
                "ResourceGovernancePolicyDescription.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION*)nativePtr);

            var resourceGovPolicyDesc =  new ResourceGovernancePolicyDescription
            {
                CodePackageRef = NativeTypes.FromNativeString(nativeDescription.CodePackageRef),
                MemoryInMB = nativeDescription.MemoryInMB,
                MemorySwapInMB = nativeDescription.MemorySwapInMB,
                MemoryReservationInMB = nativeDescription.MemoryReservationInMB,
                CpuShares = nativeDescription.CpuShares,
                CpuPercent = nativeDescription.CpuPercent,
                MaximumIOps = nativeDescription.MaximumIOps,
                MaximumIOBytesps = nativeDescription.MaximumIOBytesps,
                BlockIOWeight = nativeDescription.BlockIOWeight,
                CpusetCpus = NativeTypes.FromNativeString(nativeDescription.CpusetCpus),
                NanoCpus = nativeDescription.NanoCpus,
                CpuQuota = nativeDescription.CpuQuota
            };

            if(nativeDescription.Reserved != null)
            {
                var nativeParametersEx1 = *((NativeTypes.FABRIC_RESOURCE_GOVERNANCE_POLICY_DESCRIPTION_EX1*)nativeDescription.Reserved);
                resourceGovPolicyDesc.DiskQuotaInMB = nativeParametersEx1.DiskQuotaInMB;
                resourceGovPolicyDesc.KernelMemoryInMB = nativeParametersEx1.KernelMemoryInMB;
                resourceGovPolicyDesc.ShmSizeInMB = nativeParametersEx1.ShmSizeInMB;
            }

            return resourceGovPolicyDesc;
        }
    }
}
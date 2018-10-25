// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ServicePackageResourceGovernanceDescription
    {
        internal ServicePackageResourceGovernanceDescription()
        {
        }

        internal bool IsGoverned { get; set; } 

        internal UInt32 MemoryInMB { get; set; }

        internal double CpuCores { get; set; }

        internal UInt32 NotUsed { get; set; }

        internal static unsafe ServicePackageResourceGovernanceDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(
                nativePtr != IntPtr.Zero,
                "ServicePackageResourceGovernanceDescription.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_SERVICE_PACKAGE_RESOURCE_GOVERNANCE_DESCRIPTION*)nativePtr);

            return new ServicePackageResourceGovernanceDescription
            {
                IsGoverned = NativeTypes.FromBOOLEAN(nativeDescription.IsGoverned),
                MemoryInMB = nativeDescription.MemoryInMB,
                CpuCores = nativeDescription.CpuCores,
                NotUsed = nativeDescription.NotUsed
            };
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerLogConfigDescription 
    {
        internal ContainerLogConfigDescription()
        {
        }

        internal string Driver { get; set; }

        internal List<ContainerDriverOptionDescription> DriverOpts { get; set; }

        internal static unsafe ContainerLogConfigDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(
                nativePtr != IntPtr.Zero,
                "ContainerLogConfigDescription.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_CONTAINER_LOG_CONFIG_DESCRIPTION*)nativePtr);

            return new ContainerLogConfigDescription
            {
                Driver = NativeTypes.FromNativeString(nativeDescription.Driver),
                DriverOpts = ContainerDriverOptionDescription.CreateFromNativeList(nativeDescription.DriverOpts)
            };
        }
    }
}
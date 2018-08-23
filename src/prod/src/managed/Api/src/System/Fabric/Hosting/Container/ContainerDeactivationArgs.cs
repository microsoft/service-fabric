// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;
    
    internal sealed class ContainerDeactivationArgs
    {
        internal ContainerDeactivationArgs()
        {
        }

        internal string ContainerName { get; set; }

        internal bool ConfiguredForAutoRemove { get; set; }

        internal bool IsContainerRoot { get; set; }

        internal string CgroupName { get; set; }

        internal bool IsGracefulDeactivation { get; set; }

        internal static unsafe ContainerDeactivationArgs CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerDeactivationArgs.CreateFromNative() has null pointer.");

            var nativeArgs = *((NativeTypes.FABRIC_CONTAINER_DEACTIVATION_ARGS*)nativePtr);

            return new ContainerDeactivationArgs
            {
                ContainerName = NativeTypes.FromNativeString(nativeArgs.ContainerName),
                ConfiguredForAutoRemove = NativeTypes.FromBOOLEAN(nativeArgs.ConfiguredForAutoRemove),
                IsContainerRoot = NativeTypes.FromBOOLEAN(nativeArgs.IsContainerRoot),
                CgroupName = NativeTypes.FromNativeString(nativeArgs.CgroupName),
                IsGracefulDeactivation = NativeTypes.FromBOOLEAN(nativeArgs.IsGracefulDeactivation)
            };
        }
    }
}
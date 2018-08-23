// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ContainerActivationArgs
    {
        internal ContainerActivationArgs()
        {
        }

        internal bool IsUserLocalSystem { get; set; }

        internal string AppHostId { get; set; }

        internal string NodeId { get; set; }

        internal ContainerDescription ContainerDescription { get; set; }

        internal ProcessDescription ProcessDescription { get; set; }

        internal string FabricBinPath { get; set; }

        internal string GatewayIpAddress { get; set; }

        internal static unsafe ContainerActivationArgs CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerActivationArgs.CreateFromNative() has null pointer.");

            var nativeArgs = *((NativeTypes.FABRIC_CONTAINER_ACTIVATION_ARGS*)nativePtr);

            return new ContainerActivationArgs
            {
                IsUserLocalSystem = NativeTypes.FromBOOLEAN(nativeArgs.IsUserLocalSystem),
                AppHostId = NativeTypes.FromNativeString(nativeArgs.AppHostId),
                NodeId = NativeTypes.FromNativeString(nativeArgs.NodeId),
                ContainerDescription = ContainerDescription.CreateFromNative(nativeArgs.ContainerDescription),
                ProcessDescription = ProcessDescription.CreateFromNative(nativeArgs.ProcessDescription),
                FabricBinPath = NativeTypes.FromNativeString(nativeArgs.FabricBinPath),
                GatewayIpAddress = NativeTypes.FromNativeString(nativeArgs.GatewayIpAddress)
            };
        }
    }
}
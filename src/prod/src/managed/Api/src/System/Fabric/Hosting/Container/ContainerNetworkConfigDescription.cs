// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ContainerNetworkConfigDescription
    {
        internal ContainerNetworkConfigDescription()
        {
        }

        internal string OpenNetworkAssignedIp { get; set; }

        internal IDictionary<string, string> OverlayNetworkResources { get; set; }

        internal IDictionary<string, string> PortBindings { get; set; }

        internal string NodeId { get; set; }

        internal string NodeName { get; set; }

        internal string NodeIpAddress { get; set; }

        public ContainerNetworkType NetworkType { get; set; }

        internal static unsafe ContainerNetworkConfigDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerNetworkConfigDescription.CreateFromNative() has null pointer.");

            var nativeArgs = *((NativeTypes.FABRIC_CONTAINER_NETWORK_CONFIG_DESCRIPTION*)nativePtr);

            return new ContainerNetworkConfigDescription
            {
                OpenNetworkAssignedIp = NativeTypes.FromNativeString(nativeArgs.OpenNetworkAssignedIp),
                OverlayNetworkResources = NativeTypes.FromNativeStringPairList(nativeArgs.OverlayNetworkResources),
                PortBindings = NativeTypes.FromNativeStringPairList(nativeArgs.PortBindings),
                NodeId = NativeTypes.FromNativeString(nativeArgs.NodeId),
                NodeName = NativeTypes.FromNativeString(nativeArgs.NodeName),
                NodeIpAddress = NativeTypes.FromNativeString(nativeArgs.NodeIpAddress),
                NetworkType = InteropHelpers.FromNativeContainerNetworkType(nativeArgs.NetworkType)
            };
        }
    }
}
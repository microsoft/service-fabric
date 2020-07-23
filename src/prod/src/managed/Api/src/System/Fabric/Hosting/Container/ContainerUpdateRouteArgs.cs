// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Collections.Generic;
    using System.Fabric.Interop;

    internal sealed class ContainerUpdateRouteArgs
    {
        internal ContainerUpdateRouteArgs()
        {
        }

        internal string ContainerId { get; set; }

        internal string ContainerName { get; set; }

        internal string ApplicationId { get; set; }

        internal string ApplicationName { get; set; }

        public ContainerNetworkType NetworkType { get; set; }

        internal List<string> GatewayIpAddresses { get; set; }

        internal bool AutoRemove { get; set; }

        internal bool IsContainerRoot { get; set; }

        internal string CgroupName { get; set; }

        internal static unsafe ContainerUpdateRouteArgs CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerUpdateRouteArgs.CreateFromNative() has null pointer.");

            var nativeUpdateRouteArgs = *((NativeTypes.FABRIC_CONTAINER_UPDATE_ROUTE_ARGS*)nativePtr);

            return new ContainerUpdateRouteArgs
            {
                ContainerId = NativeTypes.FromNativeString(nativeUpdateRouteArgs.ContainerId),
                ContainerName = NativeTypes.FromNativeString(nativeUpdateRouteArgs.ContainerName),
                ApplicationId = NativeTypes.FromNativeString(nativeUpdateRouteArgs.ApplicationId),
                ApplicationName = NativeTypes.FromNativeString(nativeUpdateRouteArgs.ApplicationName),
                NetworkType = InteropHelpers.FromNativeContainerNetworkType(nativeUpdateRouteArgs.NetworkType),
                GatewayIpAddresses = NativeTypes.FromNativeStringList(nativeUpdateRouteArgs.GatewayIpAddresses),
                AutoRemove = NativeTypes.FromBOOLEAN(nativeUpdateRouteArgs.AutoRemove),
                IsContainerRoot = NativeTypes.FromBOOLEAN(nativeUpdateRouteArgs.IsContainerRoot),
                CgroupName = NativeTypes.FromNativeString(nativeUpdateRouteArgs.CgroupName)
            };
        }
    }
}

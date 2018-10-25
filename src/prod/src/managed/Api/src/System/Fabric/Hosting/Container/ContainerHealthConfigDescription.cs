// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ContainerHealthConfigDescription
    {
        internal ContainerHealthConfigDescription()
        {
        }

        internal bool IncludeDockerHealthStatusInSystemHealthReport { get; set; }

        internal bool RestartContainerOnUnhealthyDockerHealthStatus { get; set; }

        internal static unsafe ContainerHealthConfigDescription CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerActivationArgs.CreateFromNative() has null pointer.");

            var nativeDescription = *((NativeTypes.FABRIC_CONTAINER_HEALTH_CONFIG_DESCRIPTION*)nativePtr);

            return new ContainerHealthConfigDescription
            {
                IncludeDockerHealthStatusInSystemHealthReport = NativeTypes.FromBOOLEAN(nativeDescription.IncludeDockerHealthStatusInSystemHealthReport),
                RestartContainerOnUnhealthyDockerHealthStatus = NativeTypes.FromBOOLEAN(nativeDescription.RestartContainerOnUnhealthyDockerHealthStatus)
            };
        }
    }
}
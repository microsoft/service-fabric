// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;

    internal static class FabricTransportSettingsExtension
    {
        internal static IntPtr ToNativeV2(this FabricTransportSettings transportSettings, PinCollection pin)
        {
            var nativeObj = new NativeFabricTransport.FABRIC_TRANSPORT_SETTINGS();
            nativeObj.Reserved = IntPtr.Zero;

            if (transportSettings.SecurityCredentials != null)
            {
                nativeObj.SecurityCredentials = transportSettings.SecurityCredentials.ToNative(pin);
            }
            else
            {
                nativeObj.SecurityCredentials = IntPtr.Zero;
            }

            if (transportSettings.OperationTimeout.TotalSeconds < 0)
            {
                nativeObj.OperationTimeoutInSeconds = 0;
            }
            else
            {
                nativeObj.OperationTimeoutInSeconds = (uint) transportSettings.OperationTimeout.TotalSeconds;
            }

            if (transportSettings.KeepAliveTimeout.TotalSeconds < 0)
            {
                nativeObj.KeepAliveTimeoutInSeconds = 0;
            }
            else
            {
                nativeObj.KeepAliveTimeoutInSeconds = (uint) transportSettings.KeepAliveTimeout.TotalSeconds;
            }


            Helper.ThrowIfValueOutOfBounds(transportSettings.MaxMessageSize, "MaxMessageSize");

            nativeObj.MaxMessageSize = (uint) transportSettings.MaxMessageSize;

            Helper.ThrowIfValueOutOfBounds(transportSettings.MaxConcurrentCalls, "MaxConcurrentCalls");
            nativeObj.MaxConcurrentCalls = (uint) transportSettings.MaxConcurrentCalls;

            Helper.ThrowIfValueOutOfBounds(transportSettings.MaxQueueSize, "MaxQueueSize");

            nativeObj.MaxQueueSize = (uint) transportSettings.MaxQueueSize;

            return pin.AddBlittable(nativeObj);
        }
    }
}
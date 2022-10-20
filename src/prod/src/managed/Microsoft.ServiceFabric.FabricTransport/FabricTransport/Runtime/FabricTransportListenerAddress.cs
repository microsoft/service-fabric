// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.Runtime
{
    using System;
    using System.Fabric;
    using System.Fabric.Interop;

    internal class FabricTransportListenerAddress
    {
        public string Path { get; set; }

        public string IpAddressOrFQDN { get; set; }

        public int Port { get; set; }

        public FabricTransportListenerAddress(string ipAddressOrFQDN, int port, string path)
        {
            this.IpAddressOrFQDN = ipAddressOrFQDN;
            this.Port = port;
            this.Path = path;
        }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeObj = new NativeTypes.FABRIC_SERVICE_LISTENER_ADDRESS();
            if (string.IsNullOrEmpty(this.IpAddressOrFQDN))
            {
                throw new FabricInvalidAddressException("IpAddressOrFQDN is Null or Empty");
            }

            nativeObj.IPAddressOrFQDN = pin.AddBlittable(this.IpAddressOrFQDN);

            Helper.ThrowIfValueOutOfBounds(this.Port, "Port");
            nativeObj.Port = (uint) this.Port;

            if (string.IsNullOrEmpty(this.Path))
            {
                throw new FabricInvalidAddressException("Service Path is Null or Empty");
            }

            nativeObj.Path = pin.AddBlittable(this.Path);
            return pin.AddBlittable(nativeObj);
        }
    }
}
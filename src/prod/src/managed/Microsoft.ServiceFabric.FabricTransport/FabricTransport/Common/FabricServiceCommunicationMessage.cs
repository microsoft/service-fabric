// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport
{
    using System;
    using System.Fabric.Interop;

    internal class FabricServiceCommunicationMessage : NativeServiceCommunication.IFabricServiceCommunicationMessage,
        IDisposable
    {
        private PinCollection pin;
        private readonly IntPtr nativeHeaders;
        private readonly IntPtr nativeBody;

        public FabricServiceCommunicationMessage(byte[] headers, byte[] body)
        {
            // Create pinned objects for header and body.
            this.pin = new PinCollection();
            var nativeObj = new NativeTypes.FABRIC_MESSAGE_BUFFER();
            var nativeValue = NativeTypes.ToNativeBytes(this.pin, headers);
            nativeObj.BufferSize = nativeValue.Item1;
            nativeObj.Buffer = nativeValue.Item2;
            this.nativeHeaders = this.pin.AddBlittable(nativeObj);

            nativeObj = new NativeTypes.FABRIC_MESSAGE_BUFFER();
            nativeValue = NativeTypes.ToNativeBytes(this.pin, body);
            nativeObj.BufferSize = nativeValue.Item1;
            nativeObj.Buffer = nativeValue.Item2;
            this.nativeBody = this.pin.AddBlittable(nativeObj);
        }

        public IntPtr Get_Body()
        {
            return this.nativeBody;
        }

        public IntPtr Get_Headers()
        {
            return this.nativeHeaders;
        }

        #region IDisposable Support

        private bool disposedValue;

        protected virtual void Dispose(bool disposing)
        {
            if (!this.disposedValue)
            {
                if (disposing)
                {
                    if (this.pin != null)
                    {
                        this.pin.Dispose();
                        this.pin = null;
                    }
                }

                this.disposedValue = true;
            }
        }

        ~FabricServiceCommunicationMessage()
        {
            this.Dispose(false);
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        #endregion
    }
}
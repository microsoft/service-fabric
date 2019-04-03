// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Linq;
    using System.Runtime.InteropServices;

    internal class NativeFabricTransportMessage : NativeFabricTransport.IFabricTransportMessage
    {
        private PinCollection pin;
        private readonly IntPtr nativeHeadersPtr;
        private readonly IntPtr nativeBodyBuffersPtr;
        private bool disposedValue;
        private readonly FabricTransportMessage message;
        private int length;
        public NativeFabricTransportMessage(FabricTransportMessage message)
        {
            this.message = message;
            // Create pinned objects for header and body.
            this.pin = new PinCollection();
            var nativeHeaderBuffer = this.CreateNativeHeaderBytes();
            this.nativeHeadersPtr = this.pin.AddBlittable(nativeHeaderBuffer);
            this.nativeBodyBuffersPtr = this.CreateNativeBodyBytesPtr();
        }

        public static FabricTransportMessage ToFabricTransportMessage(
            NativeFabricTransport.IFabricTransportMessage message)
        {
            FabricTransportMessage msg;
            uint count;
            IntPtr headerBuffer;
            IntPtr messageBuffer;
            message.GetHeaderAndBodyBuffer(out headerBuffer,
                out count,
                out messageBuffer);
            if (headerBuffer != IntPtr.Zero)
            {
                msg =
                    new FabricTransportMessage(
                        NativeMessageToFabricTransportHeader(headerBuffer),
                        NativeMessageToFabricTransportBody(count, messageBuffer),
                        message);

            }
            else
            {
                msg = new FabricTransportMessage(null,
                    NativeMessageToFabricTransportBody(count, messageBuffer), 
                    message);

            }

            GC.KeepAlive(message);
            return msg;
        }

        private static FabricTransportRequestBody NativeMessageToFabricTransportBody(
           uint count, IntPtr messageBuffer)
        {
            if (count > 0)
            {
                var stream = new NativeMessageStream(GetBuffersPtr(count, messageBuffer));
                return new FabricTransportRequestBody(stream);
            }
            return null;
        }

        private static FabricTransportRequestHeader NativeMessageToFabricTransportHeader(
             IntPtr headerBuffer)
        {
            var stream = new NativeMessageStream(GetHeaderBuffersPtr(headerBuffer));
            return new FabricTransportRequestHeader(stream);
        }


        public static unsafe byte[] GetBytesFromNative(IntPtr ptr)
        {
            var nativeBodyBuffers = (NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER*) ptr;
            return NativeTypes.FromNativeBytes(nativeBodyBuffers->Buffer, nativeBodyBuffers->BufferSize);
        }

        private static unsafe List<Tuple<uint, IntPtr>> GetBuffersPtr(
           uint count, IntPtr messageBuffer)
        {
          var bufferList = new List<Tuple<uint, IntPtr>>((int) count);
            for (var i = 0; i < count; i++)
            {
                var msgBuffer =
                    (NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER*)
                        (messageBuffer + i*Marshal.SizeOf(typeof(NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER)));
                var tuple = new Tuple<uint, IntPtr>(msgBuffer->BufferSize, msgBuffer->Buffer);
                bufferList.Add(tuple);
            }

            return bufferList;
        }

        private static unsafe List<Tuple<uint, IntPtr>> GetHeaderBuffersPtr(
            IntPtr headerPtr)
        {
            var buffer = (NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER*)headerPtr;
            var bufferList = new List<Tuple<uint, IntPtr>>(1);
            
            var tuple = new Tuple<uint, IntPtr>(buffer->BufferSize, buffer->Buffer);
            bufferList.Add(tuple);

            return bufferList;
        }

        internal IntPtr CreateNativeBodyBytesPtr()
        {
            if (this.message.GetBody() == null || !this.message.GetBody().GetBodyBuffers().Any())
            {
                var messageBuffer = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER();
                messageBuffer.BufferSize = 0;
                messageBuffer.Buffer = IntPtr.Zero;
                this.length = 0;
                return this.pin.AddBlittable(messageBuffer);
            }
            else
            {
                var nativeBuffer = this.CreateNativeBodyBytes();
                return this.pin.AddBlittable(nativeBuffer);
            }
        }

        internal NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER[] CreateNativeBodyBytes()
        {
            var bodyBuffer = this.message.GetBody().GetBodyBuffers();
            this.length = bodyBuffer.Count();
            var bufferarray = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER[this.length];
            int i = 0;
            foreach (var item in bodyBuffer)
            {
                bufferarray[i].BufferSize = (uint) item.Count;
                var bufferAddress = this.pin.AddBlittable(item.Array);
                bufferarray[i].Buffer = IntPtr.Add(bufferAddress, sizeof(byte)*item.Offset);
                i++;
            }

            return bufferarray;
        }

        internal NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER CreateNativeHeaderBytes()
        {
            if (this.message.GetHeader() == null || this.message.GetHeader().GetSendBuffer() == null)
            {
                var emptyNativebuffer = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER();
                emptyNativebuffer.BufferSize = 0;
                emptyNativebuffer.Buffer = IntPtr.Zero;
                return emptyNativebuffer;
            }
            var nativeObj = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER();
            var headers = this.message.GetHeader().GetSendBuffer();
            nativeObj.BufferSize = (uint) headers.Count;
            var bufferAddress = this.pin.AddBlittable(headers.Array);
            nativeObj.Buffer = IntPtr.Add(bufferAddress, sizeof(byte)*headers.Offset);

            return nativeObj;
        }

        internal NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER CreateNativeHeaderBytes(byte[] bytes)
        {
            if (bytes == null)
            {
                var emptyNativebuffer = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER();
                emptyNativebuffer.BufferSize = 0;
                emptyNativebuffer.Buffer = IntPtr.Zero;
                return emptyNativebuffer;
            }
            var nativeObj = new NativeFabricTransport.FABRIC_TRANSPORT_MESSAGE_BUFFER();
            nativeObj.BufferSize = (uint) bytes.Length;
            nativeObj.Buffer = this.pin.AddBlittable(bytes);
            return nativeObj;
        }

        #region IDisposable Support

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

                    this.message.Dispose();
                }
                this.disposedValue = true;
            }
        }
        
        public void GetHeaderAndBodyBuffer(out IntPtr HeaderPtr, out uint bufferlength, out IntPtr bufferPtr)
        {
            bufferPtr = this.nativeBodyBuffersPtr;
            bufferlength = (uint)this.length;
            HeaderPtr = this.nativeHeadersPtr;
        }


        public void Dispose()
        {
            this.Dispose(true);
        }

        #endregion
 
    }
}
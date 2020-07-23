// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.FabricTransport.V2
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Runtime.InteropServices;

    internal class FabricTransportMessage : IDisposable
    {
        private readonly FabricTransportRequestHeader requestHeader;
        private readonly FabricTransportRequestBody requestBody;
        private readonly object nativeInterfaceRoot;

        public FabricTransportMessage(FabricTransportRequestHeader requestHeader, FabricTransportRequestBody requestBody)
        {
            this.requestHeader = requestHeader;
            this.requestBody = requestBody;
            this.nativeInterfaceRoot = null;
        }

        public FabricTransportMessage(FabricTransportRequestHeader requestHeader,
            FabricTransportRequestBody requestBody,
            object nativeInterfaceRoot)
        {
            this.requestHeader = requestHeader;
            this.requestBody = requestBody;
            this.nativeInterfaceRoot = nativeInterfaceRoot;
        }

        public FabricTransportRequestBody GetBody()
        {
            return this.requestBody;
        }

        public FabricTransportRequestHeader GetHeader()
        {
            return this.requestHeader;
        }

        public void Dispose()
        {
            if (this.nativeInterfaceRoot != null)
            {
                //To Make it work for Managed CIT's
                if (Marshal.IsComObject(this.nativeInterfaceRoot))
                {
                    Marshal.ReleaseComObject(this.nativeInterfaceRoot);
                }
            }

            if (this.requestBody != null)
            {
                this.requestBody.Dispose();
            }
            if (this.requestHeader != null)
            {
                this.requestHeader.Dispose();
            }
        }
    }

    internal class FabricTransportRequestHeader
    {
        private readonly Stream recievedHeaderStream;
        private readonly ArraySegment<byte> requestHeaderBuffer;
        private readonly Action disposeAction;

        public ArraySegment<byte> GetSendBuffer()
        {
            return this.requestHeaderBuffer;
        }

        public FabricTransportRequestHeader(ArraySegment<byte> requestHeaderBuffer, Action disposeAction)
        {
            this.requestHeaderBuffer = requestHeaderBuffer;
            this.disposeAction = disposeAction;
        }

        public FabricTransportRequestHeader(Stream recievedHeaderStream)
        {
            this.recievedHeaderStream = recievedHeaderStream;
        }

        public Stream GetRecievedStream()
        {
            return this.recievedHeaderStream;
        }

        public void Dispose()
        {
            if (this.disposeAction != null)
            {
                this.disposeAction();
            }
        }
    }

    internal class FabricTransportRequestBody
    {
        private readonly IEnumerable<ArraySegment<byte>> sendBuffers;
        private readonly Action disposeAction;
        private readonly Stream recievedStream;

        public IEnumerable<ArraySegment<byte>> GetBodyBuffers()
        {
            return this.sendBuffers;
        }

        public FabricTransportRequestBody(IEnumerable<ArraySegment<byte>> sendBuffers, Action disposeAction)
        {
            this.sendBuffers = sendBuffers;
            this.disposeAction = disposeAction;
        }

        public FabricTransportRequestBody(Stream recievedStream)
        {
            this.recievedStream = recievedStream;
        }

        public Stream GetRecievedStream()
        {
            return this.recievedStream;
        }

        public void Dispose()
        {
            if (this.disposeAction != null)
            {
                this.disposeAction();
            }
        }
    }
}
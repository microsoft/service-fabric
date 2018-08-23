// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ContainerApiExecutionResponse
    {
        internal ContainerApiExecutionResponse()
        {
            this.ContentType = string.Empty;
            this.ContentEncoding = string.Empty;
        }

        internal UInt16 StatusCode { get; set; }

        internal string ContentType { get; set; }

        internal string ContentEncoding { get; set; }

        internal byte[] ResponseBody { get; set; }

        internal IntPtr ToNative(PinCollection pin)
        {
            var nativeResponseBody = NativeTypes.ToNativeBytes(pin, this.ResponseBody);

            var nativeExecutionResponse = new NativeTypes.FABRIC_CONTAINER_API_EXECUTION_RESPONSE
            {
                StatusCode = this.StatusCode,
                ContentType = pin.AddBlittable(this.ContentType),
                ContentEncoding = pin.AddBlittable(this.ContentEncoding),
                ResponseBodyBufferSize = nativeResponseBody.Item1,
                ResponseBody = nativeResponseBody.Item2
            };

            return pin.AddBlittable(nativeExecutionResponse);
        }
    }
}
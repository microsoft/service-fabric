// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ContainerApiExecutionArgs
    {
        internal ContainerApiExecutionArgs()
        {
        }

        internal string ContainerName { get; set; }

        internal string HttpVerb { get; set; }

        internal string UriPath { get; set; }

        internal string ContentType { get; set; }

        internal string RequestBody { get; set; }

        internal static unsafe ContainerApiExecutionArgs CreateFromNative(IntPtr nativePtr)
        {
            ReleaseAssert.AssertIfNot(nativePtr != IntPtr.Zero, "ContainerApiExecutionArgs.CreateFromNative() has null pointer.");

            var nativeArgs = *((NativeTypes.FABRIC_CONTAINER_API_EXECUTION_ARGS*)nativePtr);

            return new ContainerApiExecutionArgs
            {
                ContainerName = NativeTypes.FromNativeString(nativeArgs.ContainerName),
                HttpVerb = NativeTypes.FromNativeString(nativeArgs.HttpVerb),
                UriPath = NativeTypes.FromNativeString(nativeArgs.UriPath),
                ContentType = NativeTypes.FromNativeString(nativeArgs.ContentType),
                RequestBody = NativeTypes.FromNativeString(nativeArgs.RequestBody)
            };
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    internal class NativeNoneCredentialConverter : INativeCredentialConverter
    {
        public IntPtr ToNative(PinCollection pin)
        {
            return IntPtr.Zero;
        }
    }
}
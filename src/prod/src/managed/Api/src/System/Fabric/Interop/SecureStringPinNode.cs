// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Runtime.InteropServices;
    using System.Security;

    internal sealed class SecureStringPinNode : IPinNode
    {
        private IntPtr stringPtr;

        public SecureStringPinNode(SecureString secureString)
        {
            if (secureString == null)
            {
                throw new ArgumentNullException(nameof(secureString));
            }

            this.stringPtr = IntPtr.Zero;

#if !DotNetCoreClr
            this.stringPtr = Marshal.SecureStringToGlobalAllocUnicode(secureString);
#else
            this.stringPtr = SecureStringMarshal.SecureStringToGlobalAllocUnicode(secureString);
#endif
        }

        ~SecureStringPinNode()
        {
            this.Dispose(false);
        }

        public IntPtr AddrOfPinnedObject()
        {
            return this.stringPtr;
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (this.stringPtr != IntPtr.Zero)
            {
                Marshal.ZeroFreeGlobalAllocUnicode(this.stringPtr);
                this.stringPtr = IntPtr.Zero;
            }
        }
    }
}

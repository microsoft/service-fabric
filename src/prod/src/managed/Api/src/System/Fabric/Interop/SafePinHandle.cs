// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System;
    using System.Runtime.InteropServices;

    internal sealed class SafePinHandle : SafeHandle
    {
        public SafePinHandle(object target)
            : base(IntPtr.Zero, true)
        {
            if (target != null)
            {
                IntPtr p = IntPtr.Zero;

                try
                {
                    p = GCHandle.ToIntPtr(GCHandle.Alloc(target, GCHandleType.Pinned));
                }
                finally
                {
                    if (p != IntPtr.Zero)
                    {
                        this.SetHandle(p);
                    }
                }
            }
        }

        public override bool IsInvalid
        {
            get { return this.handle == IntPtr.Zero; }
        }

        public IntPtr AddrOfPinnedObject()
        {
            if (this.IsInvalid)
            {
                return IntPtr.Zero;
            }
            else
            {
                return GCHandle.FromIntPtr(this.handle).AddrOfPinnedObject();
            }
        }

        protected override bool ReleaseHandle()
        {
            GCHandle.FromIntPtr(this.handle).Free();
            return true;
        }
    }
}
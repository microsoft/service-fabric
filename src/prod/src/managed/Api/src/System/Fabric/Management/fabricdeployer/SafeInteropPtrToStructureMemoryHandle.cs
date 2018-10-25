// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Runtime.InteropServices;

    public class SafeInteropPtrToStructureMemoryHandle<T> : SafeHandle
    {
        public T Struct
        {
            get
            {
                return (T)Marshal.PtrToStructure(handle, typeof(T));
            }
        }

        public SafeInteropPtrToStructureMemoryHandle() : base(IntPtr.Zero, true) { }

        public SafeInteropPtrToStructureMemoryHandle(int size) : base(IntPtr.Zero, true)
        {
            handle = Marshal.AllocHGlobal(size);
        }

        public override bool IsInvalid
        {
            get
            {
                return handle == IntPtr.Zero;
            }
        }

        protected override bool ReleaseHandle()
        {
            if (handle != IntPtr.Zero)
            {
                Marshal.DestroyStructure(handle, typeof(T));
                Marshal.FreeHGlobal(handle);
                handle = IntPtr.Zero;
                return true;
            }

            return false;
        }

        public Int64 ToInt64()
        {
            return handle.ToInt64();
        }
    }
}
// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    using Runtime.InteropServices;

    public class SafeInteropStructureToPtrMemoryHandle : SafeHandle
    {
        private Type ObjectType { get; set; }

        public SafeInteropStructureToPtrMemoryHandle(object obj) : base(IntPtr.Zero, true)
        {
            if (obj != null)
            {
                ObjectType = obj.GetType();
                int size = Marshal.SizeOf(obj);
                handle = Marshal.AllocHGlobal(size);
                Marshal.StructureToPtr(obj, handle, false);
            }
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
                Marshal.DestroyStructure(handle, ObjectType);
                Marshal.FreeHGlobal(handle);
                handle = IntPtr.Zero;
                return true;
            }

            return false;
        }
    }
}
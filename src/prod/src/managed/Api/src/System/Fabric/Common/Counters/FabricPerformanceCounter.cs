// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;
    using System.Threading;
    using System.Runtime.InteropServices;

    internal sealed class FabricPerformanceCounter : SafeHandle
    {
        public FabricPerformanceCounter(IntPtr instanceHandle, int index) : base(IntPtr.Zero, true)
        {
            IntPtr counterHandle = Marshal.AllocHGlobal(sizeof(long));
            SetHandle(counterHandle);
            unsafe
            {
                *(long*)counterHandle = 0;
            }
            FabricPerformanceCounterInterop.SetCounterRefValue(instanceHandle, index, counterHandle);
        }

        public void Increment()
        {
            unsafe
            {
                Interlocked.Increment(ref *(long*)this.handle);
            }
        }

        public void Decrement()
        {
            unsafe

            {
                Interlocked.Decrement(ref *(long*)this.handle);
            }

        }

        public void IncrementBy(long val)
        {
            unsafe
            {
                Interlocked.Add(ref *(long*)this.handle, val);
            }
        }

        public long GetValue()
        {
            unsafe
            {
                return Interlocked.Read(ref *(long*)this.handle);
            }
        }

        public void SetValue(long val)
        {
            unsafe
            {
                Interlocked.Exchange(ref *(long*)this.handle, val);
            }
        }

        protected override bool ReleaseHandle()
        {
            //Free Counter's memory
            Marshal.FreeHGlobal(this.handle);
            return true;
        }

        public override bool IsInvalid
        {
            get { return this.handle == IntPtr.Zero; }
        }

    }
}
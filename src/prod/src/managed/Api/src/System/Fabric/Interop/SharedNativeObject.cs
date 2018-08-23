// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Interop
{
    using System.Fabric.Common;
    using System.Globalization;
    using System.Threading;

    class SharedNativeObject<T> where T : class
    {
        private T sharedNativeObject;
        private ReferenceCount referenceCount;
        private string typeName;
        private string traceType;

        public SharedNativeObject(string traceType)
        {
            this.sharedNativeObject = null;
            this.referenceCount = new ReferenceCount();
            this.typeName = string.Empty;
            this.traceType = traceType;
        }

        public bool TryInitialize(T nativeObject)
        {
            ReleaseAssert.AssertIf(nativeObject == null, "nativeObject");
            if (Interlocked.CompareExchange<T>(ref this.sharedNativeObject, nativeObject, null) == null)
            {
                this.typeName = string.Format(CultureInfo.InvariantCulture, "SharedNativeObject<{0}>-{1}", typeof(T).Name, this.GetHashCode());
                AppTrace.TraceSource.WriteNoise(this.traceType, "{0} is initialized. CurrentReferenceCount={1}", this.typeName, this.GetCurrentCount());

                return true;
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(this.traceType, "{0} is already initialized. CurrentReferenceCount={1}", this.typeName, this.GetCurrentCount());

                return false;
            }
        }

        public T TryAcquire()
        {
            ReleaseAssert.AssertIfNull(this.sharedNativeObject, "sharedNativeObject");

            if (this.referenceCount.TryAdd() > 0)
            {
                AppTrace.TraceSource.WriteNoise(this.traceType, "{0}.TryAdd: SUCCESS. CurrentReferenceCount={1}", this.typeName, this.GetCurrentCount());
        
                return this.sharedNativeObject;
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(this.traceType, "{0}.TryAdd: FAILED. CurrentReferenceCount={1}", this.typeName, this.GetCurrentCount());
                
                return null;
            }
        }

        public void Release()
        {
            ReleaseAssert.AssertIfNull(this.sharedNativeObject, "sharedNativeObject");

            AppTrace.TraceSource.WriteNoise(this.traceType, "SharedNativeObject<{0}>.Release: CurrentReferenceCount={1}", this.typeName, this.GetCurrentCount());

            if (this.referenceCount.Release() == 0)
            {
                Utility.SafeReleaseComObject(this.sharedNativeObject);
            }
        }

        public void TryAcquireAndInvoke(Action<T> action)
        {
            T obj = this.TryAcquire();
            if (obj == null)
            {
                return;
            }

            try
            {
                action(obj);
            }
            finally
            {
                this.Release();
            }
        }

        private long GetCurrentCount()
        {
            return this.referenceCount.GetCurrentValue();
        }

        /// <summary>
        /// A utility class to count references/usage. In addition to the regular add/release, conditional add is also supported.
        /// The conditional add fails if the current count is zero.
        /// </summary>
        private class ReferenceCount
        {
            private long count;

            public ReferenceCount()
            {
                this.count = 1;
            }

            public long TryAdd()
            {
                long current = 0;
                long updated = 0;
                do
                {
                    current = Interlocked.Read(ref this.count);

                    if (current == 0)
                    {
                        return current;
                    }

                    updated = current + 1;
                }
                while (Interlocked.CompareExchange(ref this.count, updated, current) != current);

                return updated;
            }

            public long Release()
            {
                var nowCount = Interlocked.Decrement(ref this.count);
                if (nowCount < 0)
                {
                    ReleaseAssert.Failfast("Decrement called on the reference count that is already zero.");
                }

                return nowCount;
            }

            public long GetCurrentValue()
            {
                return Interlocked.Read(ref this.count);
            }
        }
    }
}
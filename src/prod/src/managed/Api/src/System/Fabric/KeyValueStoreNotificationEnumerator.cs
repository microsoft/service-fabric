// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    internal sealed class KeyValueStoreNotificationEnumerator : IEnumerator<KeyValueStoreNotification>
    {
        private bool disposed;
        private NativeRuntime.IFabricKeyValueStoreNotificationEnumerator2 nativeEnumerator;

        private KeyValueStoreNotification cachedCurrent;
        private object cachedCurrentLock;

        internal KeyValueStoreNotificationEnumerator(
            NativeRuntime.IFabricKeyValueStoreNotificationEnumerator nativeEnumerator)
        {
            Utility.WrapNativeSyncInvokeInMTA(() => 
            { 
                this.nativeEnumerator = (NativeRuntime.IFabricKeyValueStoreNotificationEnumerator2)nativeEnumerator;
            },
            "KeyValueStoreNotificationEnumerator.ctor");

            this.cachedCurrent = null;
            this.cachedCurrentLock = new object();
        }

        ~KeyValueStoreNotificationEnumerator()
        {
            this.Dispose(false);
        }

        public KeyValueStoreNotification Current
        {
            get
            {
                this.ThrowIfDisposed();

                lock (this.cachedCurrentLock)
                {
                    if (this.cachedCurrent == null)
                    {
                        this.cachedCurrent = Utility.WrapNativeSyncInvoke(
                         () => this.GetCurrentHelper(),
                         "KeyValueStoreNotificationEnumerator.GetCurrent");
                    }

                    return this.cachedCurrent;
                }
            }
        }

        object Collections.IEnumerator.Current
        {
            get
            {
                return this.Current;
            }
        }

        public bool MoveNext()
        {
            this.ThrowIfDisposed();

            lock (this.cachedCurrentLock)
            {
                this.cachedCurrent = null;
            }

            return Utility.WrapNativeSyncInvoke(
             () => this.MoveNextHelper(),
             "KeyValueStoreNotificationEnumerator.MoveNext");
        }

        public void Reset()
        {
            this.nativeEnumerator.Reset();
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    this.ReleaseNativeEnumerator();
                }

                this.disposed = true;
            }
        }

        private void ReleaseNativeEnumerator()
        {
            if (this.nativeEnumerator != null)
            {
                Marshal.FinalReleaseComObject(this.nativeEnumerator);
                this.nativeEnumerator = null;
            }
        }

        private bool MoveNextHelper()
        {
            var nativeResult = this.nativeEnumerator.TryMoveNext();
            return NativeTypes.FromBOOLEAN(nativeResult);
        }

        private KeyValueStoreNotification GetCurrentHelper()
        {
            var nativeCurrentItem = this.nativeEnumerator.get_Current();
            if (nativeCurrentItem == null)
            {
                return null;
            }
            else
            {
                return KeyValueStoreNotification.CreateFromNative(nativeCurrentItem);
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("KeyValueStoreNotificationEnumerator");
            }
        }
    }
}

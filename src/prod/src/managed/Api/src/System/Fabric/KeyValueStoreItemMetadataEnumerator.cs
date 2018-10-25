// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    internal sealed class KeyValueStoreItemMetadataEnumerator : IEnumerator<KeyValueStoreItemMetadata>
    {
        private bool disposed;
        private NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator2 nativeEnumerator;
        private TransactionBase transactionBase;
        private Func<TransactionBase, NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator> createNativeEnumeratorFunc;

        private KeyValueStoreItemMetadata cachedCurrent;
        private object cachedCurrentLock;

        internal KeyValueStoreItemMetadataEnumerator(
            TransactionBase transactionBase,
            Func<TransactionBase, NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator> createNativeEnumeratorFunc)
        {
            this.transactionBase = transactionBase;
            this.createNativeEnumeratorFunc = createNativeEnumeratorFunc;

            if (this.transactionBase != null)
            {
                this.transactionBase.Link(this);
            }

            this.cachedCurrent = null;
            this.cachedCurrentLock = new object();

            this.Reset();
        }

        ~KeyValueStoreItemMetadataEnumerator()
        {
            this.Dispose(false);
        }

        public KeyValueStoreItemMetadata Current
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
                         "KeyValueStoreItemMetadataEnumerator.GetCurrent");
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
             "KeyValueStoreItemMetadataEnumerator.MoveNext");
        }

        public void Reset()
        {
            this.ReleaseNativeEnumerator();
            Utility.WrapNativeSyncInvokeInMTA(() => 
            {
                this.nativeEnumerator = (NativeRuntime.IFabricKeyValueStoreItemMetadataEnumerator2)this.createNativeEnumeratorFunc(this.transactionBase);
            },
            "KeyValueStoreItemMetadataEnumerator.Reset");
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
                    this.transactionBase = null;
                    this.createNativeEnumeratorFunc = null;
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

        private KeyValueStoreItemMetadata GetCurrentHelper()
        {
            var nativeCurrentItem = this.nativeEnumerator.get_Current();
            if (nativeCurrentItem == null)
            {
                return null;
            }
            else
            {
                return KeyValueStoreItemMetadata.CreateFromNative(nativeCurrentItem);
            }
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("KeyValueStoreItemMetadataEnumerator");
            }
        }
    }
}
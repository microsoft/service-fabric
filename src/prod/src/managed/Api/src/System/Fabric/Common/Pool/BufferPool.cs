// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    internal class SynchronizedBufferPool<T> where T : class, IDisposable
    {
        private const int MaxSize = 1073741824;

        private readonly Func<T> itemFactory;
        private readonly SynchronizedPool<T> innerPool;

        public SynchronizedBufferPool(Func<T> itemFactory, int limit = MaxSize)
        {
            if (limit > MaxSize || limit < 0)
            {
                throw new ArgumentException();
            }

            this.itemFactory = itemFactory;
            this.innerPool = new SynchronizedPool<T>(limit);
        }

        public void OnClear()
        {
            this.innerPool.Clear();
        }

        /// <summary>
        /// Take a pooled item.
        /// </summary>
        /// <returns>
        /// Returns null if the limit is passed.
        /// </returns>
        public T Take()
        {
            T item = this.innerPool.Take() ?? this.itemFactory.Invoke();

            return item;
        }

        internal void Return(T item)
        {
            if (!this.innerPool.Return(item))
            {
                item.Dispose();
            }
        }
    }
}
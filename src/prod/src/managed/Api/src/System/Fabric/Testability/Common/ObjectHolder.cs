// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Common
{
    using System.Collections.Generic;
    using System.Linq;

    internal class ObjectHolder<T>
    {
        private readonly object lockObject;
        private Dictionary<int, T> objectCache;

        private int id;

        public ObjectHolder()
        {
            this.lockObject = new object();
            this.objectCache = new Dictionary<int, T>();
        }

        public int Count
        {
            get
            {
                return this.objectCache.Count;
            }
        }

        public int StoreObject(T productObject)
        {
            lock (this.lockObject)
            {
                int identifier = this.id++;
                this.objectCache.Add(identifier, productObject);
                return identifier;
            }
        }

        public T RetrieveObject(int identifier)
        {
            T productObject = default(T);
            lock (this.lockObject)
            {
                this.objectCache.TryGetValue(identifier, out productObject);
            }

            return productObject;
        }

        public void Clear()
        {
            lock (this.lockObject)
            {
                this.objectCache.Clear();
            }
        }

        public void RemoveObject(int identifier)
        {
            lock (this.lockObject)
            {
                this.objectCache.Remove(identifier);
            }
        }

        public IDictionary<int, T> RetrieveOlderObjects(int identifier)
        {
            IDictionary<int, T> productObjects;
            lock (this.lockObject)
            {
                productObjects = (from i in this.objectCache where i.Key < identifier select i).ToDictionary((k) => k.Key, (v) => v.Value);
            }

            return productObjects;
        }

        public void RemoveOldestObject()
        {
            lock (this.lockObject)
            {
                int minimumIdentifier = (from i in this.objectCache select i.Key).Min();
                this.objectCache.Remove(minimumIdentifier);
            }
        }
    }
}


#pragma warning restore 1591
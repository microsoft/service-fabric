// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Common.MergeManager
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Comparable sorted sequence enumerator used as input to the Priority Queue.
    /// </summary>
    /// <typeparam name="T"></typeparam>
    internal class ComparableSortedSequenceEnumerator<T> : IComparable<ComparableSortedSequenceEnumerator<T>>, IEnumerator<T>
    {
        private readonly IComparer<T> keyComparer;
        private readonly IEnumerator<T> keyEnumerator;

        public ComparableSortedSequenceEnumerator(IEnumerable<T> keyEnumerable, IComparer<T> keyComparer)
        {
            this.keyEnumerator = keyEnumerable.GetEnumerator();
            this.keyComparer = keyComparer;
        }

        public int CompareTo(ComparableSortedSequenceEnumerator<T> other)
        {
            return this.keyComparer.Compare(this.Current, other.Current);
        }

        public T Current
        {
            get
            {
                return this.keyEnumerator.Current;
            }
        }

        object IEnumerator.Current
        {
            get
            {
                return this.keyEnumerator.Current;
            }
        }

        public void Dispose()
        {
        }

        public bool MoveNext()
        {
            return this.keyEnumerator.MoveNext();
        }

        public void Reset()
        {
            this.keyEnumerator.Reset();
        }
    }
}
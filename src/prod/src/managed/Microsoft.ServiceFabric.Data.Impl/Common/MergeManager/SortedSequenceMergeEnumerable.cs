// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Common.MergeManager
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;

    internal class SortedSequenceMergeEnumerable<T> : IEnumerable<T>
    {
        private const string ClassName = "SortedSequenceMergeEnumerable";

        private readonly List<IEnumerable<T>> inputList;
        private readonly IComparer<T> keyComparer;
        private readonly Func<T, bool> keyFilter;
        private readonly bool useFirstKey;
        private readonly T firstKey;
        private readonly bool useLastKey;
        private readonly T lastKey;

        public SortedSequenceMergeEnumerable(List<IEnumerable<T>> sequenceList, IComparer<T> keyComparer, Func<T, bool> keyFilter, bool useFirstKey, T firstKey, bool useLastKey, T lastKey)
        {
            Diagnostics.Assert(sequenceList != null, ClassName, "sequenceList must not be null.");
            Diagnostics.Assert(keyComparer != null, ClassName, "keyComparer must not be null.");

            this.inputList = sequenceList;
            this.keyComparer = keyComparer;
            this.keyFilter = keyFilter;

            this.useFirstKey = useFirstKey;
            this.firstKey = firstKey;

            this.useLastKey = useLastKey;
            this.lastKey = lastKey;
        }

        public IEnumerator<T> GetEnumerator()
        {
            return new SortedSequenceMergeEnumerator<T>(this.inputList, this.keyComparer, this.keyFilter, useFirstKey, firstKey, useLastKey, lastKey);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }
}
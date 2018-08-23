// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Default Comparer.
    /// </summary>
    /// <typeparam name="T">The type of objects to compare.</typeparam>
    internal class DefaultComparer<T> : IComparer<T> where T : IComparable<T>
    {
        /// <summary>
        /// The default comparer
        /// </summary>
        private readonly Comparer<T> comparer;

        /// <summary>
        /// Initializes a new instance of the <see cref="DefaultComparer{T}"/> class.
        /// </summary>
        public DefaultComparer()
        {
            this.comparer = Comparer<T>.Default;
        }

        /// <summary>
        /// Compares two specified IComparable objects and returns an integer that indicates their relative position in the sort order.
        /// </summary>
        /// <param name="x">The first T to compare.</param>
        /// <param name="y">The second T to compare.</param>
        /// <returns>A 32-bit signed integer that indicates the lexical relationship between the two comparands.</returns>
        public int Compare(T x, T y)
        {
            return this.comparer.Compare(x, y);
        }
    }
}
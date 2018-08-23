// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Collections.Generic;

    /// <summary>
    /// Long comparer.
    /// </summary>
    internal class LongComparer : IComparer<long>, IEqualityComparer<long>
    {
        /// <summary>
        /// Compares two specified Long objects and returns an integer that indicates their relative position in the sort order.
        /// </summary>
        /// <param name="x">The first long to compare.</param>
        /// <param name="y">The second long to compare.</param>
        /// <returns>A 32-bit signed integer that indicates the lexical relationship between the two comparands.</returns>
        public int Compare(long x, long y)
        {
            return x.CompareTo(y);
        }

        /// <summary>
        /// Determines whether two Long objects have the same value.
        /// </summary>
        /// <param name="x">The first long to compare.</param>
        /// <param name="y">The second long to compare.</param>
        /// <returns>
        /// true if the value of x is the same as the value of y; otherwise, false. 
        /// If both x and y are null, the method returns true.
        /// </returns>
        public bool Equals(long x, long y)
        {
            return x.Equals(y);
        }

        /// <summary>
        /// Returns the hash code for the input long.
        /// </summary>
        /// <param name="obj">The Long.</param>
        /// <returns>hash code for the input long.</returns>
        public int GetHashCode(long obj)
        {
            return obj.GetHashCode();
        }
    }
}
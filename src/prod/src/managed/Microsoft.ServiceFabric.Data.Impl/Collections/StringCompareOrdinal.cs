// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Compares two String objects by evaluating the numeric values 
    /// of the corresponding Char objects in each string.
    /// </summary>
    internal class StringCompareOrdinal : IComparer<string>, IEqualityComparer<string>
    {
        /// <summary>
        /// Compares two specified String objects and returns an integer that indicates their relative position in the sort order.
        /// </summary>
        /// <param name="x">The first string to compare.</param>
        /// <param name="y">The second string to compare.</param>
        /// <returns>A 32-bit signed integer that indicates the lexical relationship between the two comparands.</returns>
        public int Compare(string x, string y)
        {
            return string.Compare(x, y, StringComparison.Ordinal);
        }

        /// <summary>
        /// Determines whether two String objects have the same value.
        /// </summary>
        /// <param name="x">The first string to compare.</param>
        /// <param name="y">The second string to compare.</param>
        /// <returns>
        /// true if the value of x is the same as the value of y; otherwise, false. 
        /// If both x and y are null, the method returns true.
        /// </returns>
        public bool Equals(string x, string y)
        {
            return string.Equals(x, y, StringComparison.Ordinal);
        }

        /// <summary>
        /// Returns the hash code for the input string.
        /// </summary>
        /// <param name="obj">The String.</param>
        /// <returns>hash code for the input string.</returns>
        public int GetHashCode(string obj)
        {
            if (null == obj)
            {
                throw new ArgumentNullException("obj");
            }

            return obj.GetHashCode();
        }
    }
}
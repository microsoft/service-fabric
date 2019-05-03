// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;

    /// <summary>
    /// Helper class for comparing byte arrays, or byte array segments.
    /// </summary>
    internal class ByteArrayComparer : IComparer<byte[]>, IComparer<ArraySegment<byte>>, IEqualityComparer<byte[]>, IEqualityComparer<ArraySegment<byte>>
    {
        /// <summary>
        /// Single instance for reusability.
        /// </summary>
        public static readonly ByteArrayComparer Comparer = new ByteArrayComparer();

        /// <summary>
        /// Compares the two byte arrays.
        /// </summary>
        /// <returns>
        /// -1 : <paramref name="lhs"/> comes before <paramref name="rhs"/>.
        ///  0 : <paramref name="lhs"/> is equal to <paramref name="rhs"/>.
        /// +1 : <paramref name="lhs"/> comes after <paramref name="rhs"/>.
        /// </returns>
        public int Compare(byte[] lhs, byte[] rhs)
        {
            return this.Compare(new ArraySegment<byte>(lhs), new ArraySegment<byte>(rhs));
        }

        /// <summary>
        /// Compares the two byte array segments.
        /// </summary>
        /// <returns>
        /// -1 : <paramref name="lhs"/> comes before <paramref name="rhs"/>.
        ///  0 : <paramref name="lhs"/> is equal to <paramref name="rhs"/>.
        /// +1 : <paramref name="lhs"/> comes after <paramref name="rhs"/>.
        /// </returns>
        public int Compare(ArraySegment<byte> lhs, ArraySegment<byte> rhs)
        {
            var xBytes = lhs.Array;
            var xOffset = lhs.Offset;
            var xCount = lhs.Count;

            var yBytes = rhs.Array;
            var yOffset = rhs.Offset;
            var yCount = rhs.Count;

            // Compare all bytes in-order up to the common length
            for (var i = 0; i < xCount && i < yCount; i++)
            {
                var xByte = xBytes[xOffset + i];
                var yByte = yBytes[yOffset + i];

                if (xByte < yByte)
                    return -1;
                if (xByte > yByte)
                    return 1;
            }

            // All bytes up to the common length are equal.
            // If one byte array is shorter, return that one as being smaller.
            if (xCount < yCount)
                return -1;
            if (xCount > yCount)
                return 1;

            return 0;
        }

        /// <summary>
        /// Compares the two byte arrays for equality.
        /// </summary>
        /// <returns>Bool indicating whether the contents of the byte arrays are equal.</returns>
        public bool Equals(byte[] lhs, byte[] rhs)
        {
            return this.Equals(new ArraySegment<byte>(lhs), new ArraySegment<byte>(rhs));
        }

        /// <summary>
        /// Compares the two byte array segments for equality.
        /// </summary>
        /// <returns>Bool indicating whether the contents of the byte array segments are equal.</returns>
        public bool Equals(ArraySegment<byte> lhs, ArraySegment<byte> rhs)
        {
            var xBytes = lhs.Array;
            var xOffset = lhs.Offset;
            var xCount = lhs.Count;

            var yBytes = rhs.Array;
            var yOffset = rhs.Offset;
            var yCount = rhs.Count;

            // If the number of bytes aren't the same, we immediately know the arrays can't be equal.
            if (xCount != yCount)
                return false;

            // Compare the arrays byte-wise.
            for (var i = 0; i < xCount; i++)
            {
                var xByte = xBytes[xOffset + i];
                var yByte = yBytes[yOffset + i];

                if (xByte != yByte)
                    return false;
            }

            return true;
        }

        /// <summary>
        /// Gets the hash code for the byte array.
        /// </summary>
        /// <param name="bytes">Byte array.</param>
        /// <returns>Hash code.</returns>
        public int GetHashCode(byte[] bytes)
        {
            return bytes.GetHashCode();
        }

        /// <summary>
        /// Gets the hash code for the byte array segment.
        /// </summary>
        /// <param name="bytes">Byte array segment.</param>
        /// <returns>Hash code.</returns>
        public int GetHashCode(ArraySegment<byte> bytes)
        {
            return bytes.GetHashCode();
        }
    }
}
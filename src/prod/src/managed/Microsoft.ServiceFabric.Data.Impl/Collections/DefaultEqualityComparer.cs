// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Collections.Generic;

    /// <summary>
    /// Default Equality Comparer
    /// </summary>
    /// <typeparam name="T">
    /// The type of objects to compare.This type parameter is contravariant. That
    /// is, you can use either the type you specified or any type that is less derived.
    /// For more information about covariance and contravariance, see Covariance
    /// and Contravariance in Generics.
    /// </typeparam>
    internal class DefaultEqualityComparer<T> : IEqualityComparer<T>
    {
        /// <summary>
        /// The default comparer
        /// </summary>
        private readonly EqualityComparer<T> equalityComparer;

        /// <summary>
        /// Initializes a new instance of the <see cref="DefaultEqualityComparer{T}"/> class.
        /// </summary>
        public DefaultEqualityComparer()
        {
            this.equalityComparer = EqualityComparer<T>.Default;
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
        public bool Equals(T x, T y)
        {
            return this.equalityComparer.Equals(x, y);
        }

        /// <summary>
        /// Returns the hash code for the input long.
        /// </summary>
        /// <param name="obj">The Long.</param>
        /// <returns>hash code for the input long.</returns>
        public int GetHashCode(T obj)
        {
            return this.equalityComparer.GetHashCode(obj);
        }
    }
}
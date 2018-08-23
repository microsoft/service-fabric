// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Maintains 2 versions that are a part of differantial state.
    /// </summary>
    /// <typeparam name="TValue">Type of value for the item.</typeparam>
    internal class DifferentialStateVersions<TValue>
    {
        public DifferentialStateFlags flags;

        /// <summary>
        /// Gets ore sets the current version.
        /// </summary>
        public TVersionedItem<TValue> CurrentVersion { get; set; }

        /// <summary>
        /// Gets ore sets the previous version.
        /// </summary>ry>
        public TVersionedItem<TValue> PreviousVersion { get; set; }
    }
}
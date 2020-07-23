// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Dictionary kind.
    /// </summary>
    public enum DictionaryKind : byte
    {
        /// <summary>
        /// Initial value.
        /// </summary>
        None = 0,

        /// <summary>
        /// Indicates that the dictionary holds one item.
        /// </summary>
        DictionaryOfOne = 1,

        /// <summary>
        /// Indicates that dictionary holds 9 items.
        /// </summary>
        DictionaryOfNine = 2,

        /// <summary>
        /// Indicates that the dictionary holds 10 or more items.
        /// </summary>
        DictionaryOfTenOrMore = 3
    }
}
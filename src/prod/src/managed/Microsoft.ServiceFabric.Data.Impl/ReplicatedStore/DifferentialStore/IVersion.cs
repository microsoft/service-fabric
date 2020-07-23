// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Version that is carried with every key/value modification.
    /// </summary>
    internal interface IVersion
    {
        /// <summary>
        /// Bytes representing the version of the item. Opaque blob.
        /// </summary>
        byte[] Bytes { get; }
    }
}
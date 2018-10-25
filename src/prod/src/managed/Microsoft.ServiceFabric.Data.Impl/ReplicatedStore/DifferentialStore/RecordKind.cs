// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Record kind.
    /// </summary>
    internal enum RecordKind : byte
    {
        /// <summary>
        /// Indicates that the record type is an inserted version item.
        /// </summary>
        InsertedVersion = 0,

        /// <summary>
        /// Indicates that the record type is a deleted version item.
        /// </summary>
        DeletedVersion = 1,

        /// <summary>
        /// Indicates that the record type is an updated version item.
        /// </summary>
        UpdatedVersion = 2,
    }
}
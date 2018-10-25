// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Constants for TStore Table format.
    /// </summary>
    internal static class TableConstants
    {
        /// <summary>
        /// Value representing an invalid version number.
        /// </summary>
        public const int InvalidVersion = int.MinValue;

        /// <summary>
        /// Copy protocol version number.
        /// </summary>
        public const int CopyProtocolVersion = 1;

        /// <summary>
        /// SortedTable version number.
        /// </summary>
        public const int SortedTableVersion = 1;

        /// <summary>
        /// MasterTable version number.
        /// </summary>
        public const int MasterTableVersion = 1;
    }
}
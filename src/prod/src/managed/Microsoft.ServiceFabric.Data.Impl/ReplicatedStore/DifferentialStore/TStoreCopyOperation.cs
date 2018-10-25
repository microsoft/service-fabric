// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    /// <summary>
    /// Enumeration to indicate what the operation data from the TStore copy stream contains.
    /// </summary>
    internal enum TStoreCopyOperation : byte
    {
        /// <summary>
        /// The operation data contains the version (int) of the copy protocol.
        /// </summary>
        Version = 0,

        /// <summary>
        /// The operation data contains the entire metadata file.
        /// </summary>
        MetadataTable = 1,

        /// <summary>
        /// The operation data contains the start of a TStore key checkpoint file.
        /// </summary>
        StartKeyFile = 2,

        /// <summary>
        /// The operation data contains a consecutive chunk of a TStore key checkpoint file.
        /// </summary>
        WriteKeyFile = 3,

        /// <summary>
        /// Indicates the end of a TStore key checkpoint file.
        /// </summary>
        EndKeyFile = 4,

        /// <summary>
        /// The operation data contains the start of a TStore value checkpoint file.
        /// </summary>
        StartValueFile = 5,

        /// <summary>
        /// The operation data contains a consecutive chunk of a TStore value checkpoint file.
        /// </summary>
        WriteValueFile = 6,

        /// <summary>
        /// Indicates the end of a TStore value checkpoint file.
        /// </summary>
        EndValueFile = 7,

        /// <summary>
        /// Indicates the copy operation is complete.
        /// </summary>
        Complete = 8,
    }
}
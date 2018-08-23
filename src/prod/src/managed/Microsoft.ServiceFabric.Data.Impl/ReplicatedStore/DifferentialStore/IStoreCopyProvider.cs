// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Threading.Tasks;

    /// <summary>
    /// Interface for supporting copy operations of store checkpoint files.
    /// </summary>
    internal interface IStoreCopyProvider
    {
        /// <summary>
        /// Gets the working directory for the master table to be copied.
        /// </summary>
        string WorkingDirectory { get; }

        /// <summary>
        /// Gets the current metadata table to be copied.
        /// </summary>
        Task<MetadataTable> GetMetadataTableAsync();
    }
}
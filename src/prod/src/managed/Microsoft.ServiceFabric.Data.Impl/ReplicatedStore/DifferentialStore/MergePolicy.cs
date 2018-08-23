// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    internal enum MergePolicy : int
    {
        /// <summary>
        /// Disables merge.
        /// </summary>
        None =                  0,

        /// <summary>
        /// Flag used to enable Invalid Entries Merge Policy
        /// </summary>
        InvalidEntries =        1,

        /// <summary>
        /// Flag used to enable Deleted Entries Merge Policy
        /// </summary>
        DeletedEntries =        1 << 1,

        /// <summary>
        /// Flag used to enable File Count Merge Policy
        /// </summary>
        FileCount =             1 << 2,

        /// <summary>
        /// Flag used to enable Size on Disk Merge Policy
        /// This policy decides to merge if the total size on disk exceeds a certain threshold and merges
        /// all of them if there there is at least 1 invalid or deleted entry among them.
        /// </summary>
        SizeOnDisk =            1 << 3,

        /// <summary>
        ///  Flag used to enable GDPR Merge Policy i.e. all deletes older than 30 days (configuarable)
        ///  must be removed from checkpoint files
        /// </summary>
        GDPR =                  1 << 4,

        /// <summary>
        /// Enables all merge policies.
        /// </summary>
        All =                   int.MaxValue
    }
}
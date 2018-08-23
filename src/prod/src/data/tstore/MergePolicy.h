// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data 
{
    namespace TStore
    {
        enum MergePolicy : int
        {
            // Disables merge.
            None = 0,

            // Flag used to enable Invalid Entries Merge Policy
            InvalidEntries = 1,

            // Flag used to enable Deleted Entries Merge Policy
            DeletedEntries = 1 << 1,

            // Flag used to enable File Count Merge Policy
            FileCount = 1 << 2,

            // Flag used to enable Size on Disk Merge Policy
            // This policy decides to merge if the total size on disk exceeds a certain threshold and merges
            // all of them if there there is at least 1 invalid or deleted entry among them.
            SizeOnDisk = 1 << 3,

            // Enables all merge policies.
            All = INT_MAX
        };
    }
}

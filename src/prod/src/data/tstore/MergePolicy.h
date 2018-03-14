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

            // Enables all merge policies.
            All = INT_MAX
        };
    }
}

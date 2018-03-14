// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace CopyType
    {
        enum Enum
        {
            PagedCopy = 0, // normal copy operation (all copy operations <= 4.0)
            FirstFullCopy = 1, // start a new full build (4.0, 4.1)
            FirstPartialCopy = 2, // start a new incremental build (>= 4.0)
            FirstSnapshotPartialCopy = 3, // start a new incremental build on a snapshot of the current database (>= 4.0)
            FileStreamFullCopy = 4, // (>= 4.2, deprecates old full copy protocol)
            FileStreamRebuildCopy = 5, // (>= 5.3, FileStreamFullCopy + restructuring data layout on secondary before open)

            FirstValidEnum = PagedCopy,
            LastValidEnum = FileStreamRebuildCopy
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(CopyType)
    }
}

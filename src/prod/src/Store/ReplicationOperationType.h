// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace ReplicationOperationType
    {
        enum Enum
        {
            Copy = 0,
            Insert = 1,
            Update = 2,
            Delete = 3,

            FirstValidEnum = Copy,
            LastValidEnum = Delete
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( ReplicationOperationType )
    }
}

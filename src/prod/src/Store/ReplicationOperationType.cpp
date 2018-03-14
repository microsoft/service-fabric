// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Store
{
    namespace ReplicationOperationType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Copy: w << "Copy"; return;
                case Insert: w << "Insert"; return;
                case Update: w << "Update"; return;
                case Delete: w << "Delete"; return;
            }
            w << "ReplicationOperationType(" << static_cast<int>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( ReplicationOperationType, FirstValidEnum, LastValidEnum )
    }
}


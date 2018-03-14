// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // Operation type for the StateManager.
        //
        namespace ApplyType
        {
            enum Enum
            {
                Invalid = 0,
                Insert = 1,
                Delete = 2,
                LastValidEnum = Delete
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ApplyType);
        }
    }
}

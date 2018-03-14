// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        // Serialization mode for the StateManager.
        namespace SerializationMode
        {
            enum Enum
            {
                Managed = 0,
                Native = 1,
                LastValidEnum = Native
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(SerializationMode);
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Data
{
    namespace StateManager
    {
        namespace SerializationMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Managed:
                    w << L"Managed"; return;
                case Native:
                    w << L"Native"; return;
                default:
                    Common::Assert::CodingError("Unknown SerializationMode:{0} in State Manager", val);
                }
            }

            ENUM_STRUCTURED_TRACE(SerializationMode, Managed, LastValidEnum)
        }
    }
}

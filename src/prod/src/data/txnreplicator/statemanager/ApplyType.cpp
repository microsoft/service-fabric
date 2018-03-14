// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Data
{
    namespace StateManager
    {
        namespace ApplyType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case Insert:
                    w << L"Insert"; return;
                case Delete:
                    w << L"Delete"; return;
                default:
                    Common::Assert::CodingError("Unknown ApplyType:{0} in State Manager", val);
                }
            }

            ENUM_STRUCTURED_TRACE(ApplyType, Invalid, LastValidEnum)
        }
    }
}


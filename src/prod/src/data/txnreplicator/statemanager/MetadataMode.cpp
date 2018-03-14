// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;

namespace Data
{
    namespace StateManager
    {
        namespace MetadataMode
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Active:
                    w << L"Active"; return;
                case DelayDelete:
                    w << L"DelayDelete"; return;
                case FalseProgress:
                    w << L"FalseProgress"; return;
                default:
                    Common::Assert::CodingError("Unknown MetadataMode: {0}", val);
                }
            }

            char ToChar(Enum const & val)
            {
                switch (val)
                {
                case Active:
                    return 'A';
                case DelayDelete:
                    return 'D';
                case FalseProgress:
                    return 'F';
                default:
                    Common::Assert::CodingError("Unknown MetadataMode: {0}", val);
                }
            }

            ENUM_STRUCTURED_TRACE(MetadataMode, Active, LastValidEnum)
        }
    }
}

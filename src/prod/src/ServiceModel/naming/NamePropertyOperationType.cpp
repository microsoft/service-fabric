// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    namespace NamePropertyOperationType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case CheckExistence: w << "CheckExistence"; return;
            case CheckSequence: w << "CheckSequence"; return;
            case GetProperty: w << "GetProperty"; return;
            case PutProperty: w << "PutProperty"; return;
            case DeleteProperty: w << "DeleteProperty"; return;
            case EnumerateProperties: w << "EnumerateProperties"; return;
            case PutCustomProperty: w << "PutCustomProperty"; return;
            case CheckValue: w << "CheckValue"; return;
            }

            w << "NamePropertyOperationType(" << static_cast<int>(e) << ')';
        }
    }
}

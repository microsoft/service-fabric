// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
//using namespace Reliability;
//using namespace std;

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace RepartitionType
        {
            void WriteToTextWriter(TextWriter & w, Enum const& value)
            {
                switch (value)
                {
                case Invalid:
                    w << "InvalidNone";
                    break;

                case Add:
                    w << "Add";
                    break;

                case Remove:
                    w << "Remove";
                    break;

                default:
                    w << "Unknown RepartitionType: " << static_cast<int>(value);
                    break;
                }
            }

            ENUM_STRUCTURED_TRACE(RepartitionType, Invalid, LastValidEnum)
        }
    }
}

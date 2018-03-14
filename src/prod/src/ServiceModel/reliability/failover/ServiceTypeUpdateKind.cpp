// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ServiceTypeUpdateKind
    {
        void WriteToTextWriter(TextWriter & w, Enum const& value)
        {
            switch (value)
            {
                case Disabled:
                    w << "Disabled";
                    return;
                case Enabled:
                    w << "Enabled";
                    return;
                case PartitionDisabled:
                    w << "PartitionDisabled";
                    return;
                case PartitionEnabled:
                    w << "PartitionEnabled";
                    return;
                default:
                    Assert::CodingError("unknown value for enum {0}", static_cast<int>(value));
                    return;
            }
        }

        ENUM_STRUCTURED_TRACE(ServiceTypeUpdateKind, Disabled, LastValidEnum);
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace FaultType
    {
        Enum FromPublicAPI(::FABRIC_FAULT_TYPE faultType)
        {
            switch (faultType)
            {
            case ::FABRIC_FAULT_TYPE_PERMANENT:
                return Permanent;

            case ::FABRIC_FAULT_TYPE_TRANSIENT:
                return Transient;

            default:
                return Invalid;
            };
        }

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid:
                w << "Invalid";
                break;

            case Permanent:
                w << "Permanent";
                break;

            case Transient:
                w << "Transient";
                break;

            default:
                w << "Unknown" << static_cast<int>(e);
                break;
            };
        }

        ENUM_STRUCTURED_TRACE( FaultType, Invalid, LastValidEnum );
    }
}

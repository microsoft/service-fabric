// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ReplicaOpenMode
    {
        ENUM_STRUCTURED_TRACE(ReplicaOpenMode, Invalid, LastValidEnum);

        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
                case Invalid: 
                    w << L"I"; return;
                case New: 
                    w << L"N"; return;
                case Existing: 
                    w << L"E"; return;
                default: 
                    Common::Assert::CodingError("Unknown ReplicaOpenMode value {0}", static_cast<int>(val));
            }
        }

        ::FABRIC_REPLICA_OPEN_MODE ToPublicApi(Enum toConvert)
        {
            switch(toConvert)
            {
            case Invalid:
                return ::FABRIC_REPLICA_OPEN_MODE_INVALID;
            case New:
                return ::FABRIC_REPLICA_OPEN_MODE_NEW;
            case Existing:
                return ::FABRIC_REPLICA_OPEN_MODE_EXISTING;
            default:
                Common::Assert::CodingError("Unknown ReplicaOpenMode value {0}", static_cast<int>(toConvert));
            }
        }
    }
}

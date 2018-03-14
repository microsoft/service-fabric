// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;

namespace Data
{
    namespace LoggingReplicator
    {
        namespace DrainingStream
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case Recovery:
                    w << L"Recovery"; return;
                case State:
                    w << L"State"; return;
                case Copy:
                    w << L"Copy"; return;
                case Replication:
                    w << L"Replication"; return;
                case Primary:
                    w << L"Primary"; return;
                default:
                    Common::Assert::CodingError("Unknown DrainingStream:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(DrainingStream, Invalid, LastValidEnum)
        }
    }
}

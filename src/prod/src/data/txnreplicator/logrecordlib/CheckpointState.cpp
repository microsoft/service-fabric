// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;

namespace Data
{
    namespace LogRecordLib
    {
        namespace CheckpointState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case Ready:
                    w << L"Ready"; return;
                case Applied:
                    w << L"Applied"; return;
                case Completed:
                    w << L"Completed"; return;
                case Faulted:
                    w << L"Faulted"; return;
                case Aborted:
                    w << L"Aborted"; return;
                default:
                    Common::Assert::CodingError("Unknown CheckpointState:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(CheckpointState, Invalid, LastValidEnum)
        }
    }
}

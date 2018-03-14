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
        namespace LogRecordType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case BeginTransaction:
                    w << L"BeginTransaction"; return;
                case Operation:
                    w << L"Operation"; return;
                case EndTransaction:
                    w << L"EndTransaction"; return;
                case Barrier:
                    w << L"Barrier"; return;
                case UpdateEpoch:
                    w << L"UpdateEpoch"; return;
                case Backup:
                    w << L"Backup"; return;

                case BeginCheckpoint:
                    w << L"BeginCheckpoint"; return;
                case EndCheckpoint:
                    w << L"EndCheckpoint"; return;
                case CompleteCheckpoint:
                    w << L"CompleteCheckpoint"; return;
                case Indexing:
                    w << L"Indexing"; return;
                case TruncateHead:
                    w << L"TruncateHead"; return;
                case TruncateTail:
                    w << L"TruncateTail"; return;
                case Information:
                    w << L"Information"; return;

                default:
                    CODING_ASSERT("Unknown LogRecordType:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(LogRecordType, Invalid, LastValidEnum)
        }
    }
}

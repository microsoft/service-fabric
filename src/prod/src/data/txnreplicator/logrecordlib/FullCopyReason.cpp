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
        namespace FullCopyReason
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case DataLoss:
                    w << L"DataLoss"; return;
                case InsufficientLogs:
                    w << L"InsufficientLogs"; return;
                case AtomicRedoOperationFalseProgressed:
                    w << L"AtomicRedoOperationFalseProgressed"; return;
                case Other:
                    w << L"Other"; return;
                case ProgressVectorTrimmed:
                    w << L"ProgressVectorTrimmed"; return;
                case ValidationFailed:
                    w << L"ValidationFailed"; return;
                default:
                    Common::Assert::CodingError("Unknown FullCopyReason:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(FullCopyReason, Invalid, LastValidEnum)
        }
    }
}

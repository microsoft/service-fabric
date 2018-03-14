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
        namespace CopyStage
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case CopyMetadata:
                    w << L"CopyMetadata"; return;
                case CopyNone:
                    w << L"CopyNone"; return;
                case CopyState:
                    w << L"CopyState"; return;
                case CopyProgressVector:
                    w << L"CopyProgressVector"; return;
                case CopyFalseProgress:
                    w << L"CopyFalseProgress"; return;
                case CopyScanToStartingLsn:
                    w << L"CopyScanToStartingLsn"; return;
                case CopyLog:
                    w << L"CopyLog"; return;
                case CopyDone:
                    w << L"CopyDone"; return;
                default:
                    Common::Assert::CodingError("Unknown CopyStage:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(CopyStage, Invalid, LastValidEnum)
        }
    }
}

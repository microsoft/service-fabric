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
        namespace InformationEvent
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case Invalid:
                    w << L"Invalid"; return;
                case Recovered:
                    w << L"Recovered"; return;
                case CopyFinished:
                    w << L"CopyFinished"; return;
                case ReplicationFinished:
                    w << L"ReplicationFinished"; return;
                case Closed:
                    w << L"Closed"; return;
                case PrimarySwap:
                    w << L"PrimarySwap"; return;
                case RestoredFromBackup:
                    w << L"RestoredFromBackup"; return;
                case RemovingState:
                    w << L"RemovingState"; return;
                default:
                    Common::Assert::CodingError("Unknown InformationEvent:{0} in Replicator", val);
                }
            }

            ENUM_STRUCTURED_TRACE(InformationEvent, Invalid, LastValidEnum)
        }
    }
}

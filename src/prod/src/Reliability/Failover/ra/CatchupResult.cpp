// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace CatchupResult
        {
            ENUM_STRUCTURED_TRACE(CatchupResult, NotStarted, LastValidEnum);

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                case NotStarted:
                    w << "N"; return;

                case DataLossReported:
                    w << "DL"; return;

                case CatchupCompleted:
                    w << "CC"; return;

                default:
                    Common::Assert::CodingError("Unknown CatchupResult {0}", static_cast<int>(val));
                }
            }
        }
    }
}

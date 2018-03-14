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
        namespace ProxyConfigurationStage
        {
            ENUM_STRUCTURED_TRACE(ProxyConfigurationStage, Current, LastValidEnum);

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case Current:
                        w << L"CU"; return;
                    case CurrentPending:
                        w << L"CUP"; return;
                    case Catchup: 
                        w << L"CA"; return;
                    case CatchupPending: 
                        w << L"CAP"; return;
                    case PreWriteStatusRevokeCatchup:
                        w << "WCA"; return;
                    case PreWriteStatusRevokeCatchupPending: 
                        w << "WCAP"; return;
                    default: 
                        Common::Assert::CodingError("Unknown Proxy Configuration Stage");
                }
            }
        }
    }
}

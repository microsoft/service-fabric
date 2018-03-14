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
        namespace ProxyMessageStage
        {
            ENUM_STRUCTURED_TRACE(ProxyMessageStage, None, LastValidEnum);

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case RAProxyReportFaultTransient:
                        w << L"RFT"; return;
                    case RAProxyReportFaultPermanent:
                        w << L"RFP"; return;
                    case RAProxyEndpointUpdated:
                        w << L"EU"; return;
                    case None: 
                        w << L"N"; return;
                    case RAProxyReadWriteStatusRevoked:
                        w << "RWR"; return;
                    default: 
                        Common::Assert::CodingError("Unknown Replica Message Stage");
                }
            }
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReplicaMessageStage
        {
            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case RAProxyReplyPending:
                        w << L"RAP"; return;
                    case RAReplyPending:
                        w << L"RA"; return;
                    case None: 
                        w << L"N"; return;
                    default: 
                        Common::Assert::CodingError("Unknown Replica Message Stage {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(ReplicaMessageStage, None, LastValidEnum);
        }
    }
}

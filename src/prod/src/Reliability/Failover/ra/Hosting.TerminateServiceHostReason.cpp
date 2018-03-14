// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Hosting
        {
            namespace TerminateServiceHostReason
            {
                void WriteToTextWriter(TextWriter & w, TerminateServiceHostReason::Enum const & e)
                {
                    switch (e)
                    {
                    case RemoveReplica:
                        w << "RemoveReplica"; 
                        return;

                    case FabricUpgrade:
                        w << "FabricUpgrade";
                        return;;

                    case NodeDeactivation:
                        w << "NodeDeactivation";
                        return;

                    case ApplicationUpgrade:
                        w << "ApplicationUpgrade";
                        break;

                    case NodeShutdown:
                        w << "NodeShutdown";
                        break;

                    default:
                        Assert::CodingError("Unknown reason {0}", static_cast<int>(e));
                    }
                }

                ENUM_STRUCTURED_TRACE(TerminateServiceHostReason, RemoveReplica, LastValidEnum);
            }
        }
    }
}

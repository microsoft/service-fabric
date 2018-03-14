// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace FMMessageStage
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case None:
                    w << "N";
                    break;

                case ReplicaDown:
                    w << "ReplicaDown";
                    break;

                case ReplicaUp:
                    w << "ReplicaUp";
                    break;

                case ReplicaDropped:
                    w << "ReplicaDropped";
                    break;

                case EndpointAvailable:
                    w << "Endpoint";
                    break;

                case ReplicaUpload:
                    w << "ReplicaUpload";
                    break;

                default:
                    Assert::TestAssert("unknown value {0}", static_cast<int>(e));
                    break;
                }
            }

            ENUM_STRUCTURED_TRACE(FMMessageStage, None, LastValidEnum);
        }
    }
}

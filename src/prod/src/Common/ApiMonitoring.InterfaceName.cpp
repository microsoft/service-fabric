// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;

namespace Common
{
    namespace ApiMonitoring
    {
        namespace InterfaceName
        {
            void WriteToTextWriter(TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Invalid:
                    w << "Invalid";
                    break;

                case IStatefulServiceReplica:
                    w << "IStatefulServiceReplica";
                    break;

                case IStatelessServiceInstance:
                    w << "IStatelessServiceInstance";
                    break;

                case IStateProvider:
                    w << "IStateProvider";
                    break;
                 
                case IReplicator:
                    w << "IReplicator";
                    break;

                default:
                    Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
                };
            }

            ENUM_STRUCTURED_TRACE(InterfaceName, IStatefulServiceReplica, LastValidEnum);            
        }
    }
}



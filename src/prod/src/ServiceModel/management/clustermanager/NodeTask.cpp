// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace NodeTask
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Invalid: w << "Invalid"; return;
                    case Restart: w << "Restart"; return;
                    case Relocate: w << "Relocate"; return;
                    case Remove: w << "Remove"; return;
                };

                w << "NodeTask(" << static_cast<int>(e) << ')';
            }

            ENUM_STRUCTURED_TRACE( NodeTask, Invalid, Remove );

            Enum FromPublicApi(FABRIC_NODE_TASK_TYPE publicType)
            {
                switch (publicType)
                {
                case FABRIC_NODE_TASK_TYPE_RESTART:
                    return Restart;
                    
                case FABRIC_NODE_TASK_TYPE_RELOCATE:
                    return Relocate;

                case FABRIC_NODE_TASK_TYPE_REMOVE:
                    return Remove;

                default:
                    return Invalid;
                }
            }

            FABRIC_NODE_TASK_TYPE ToPublicApi(Enum const nativeType)
            {
                switch (nativeType)
                {
                case Restart:
                    return FABRIC_NODE_TASK_TYPE_RESTART;
                    
                case Relocate:
                    return FABRIC_NODE_TASK_TYPE_RELOCATE;

                case Remove:
                    return FABRIC_NODE_TASK_TYPE_REMOVE;

                default:
                    return FABRIC_NODE_TASK_TYPE_INVALID;
                }
            }
        }
    }
}

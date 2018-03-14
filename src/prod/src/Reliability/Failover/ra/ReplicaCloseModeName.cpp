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
        namespace ReplicaCloseModeName
        {
            void WriteToTextWriter(TextWriter & w, ReplicaCloseModeName::Enum const & e)
            {
                switch (e)
                {
                case None:
                    w << "N";
                    return;

                case Close:
                    w << "Close";
                    return;

                case Drop:
                    w << "Drop";
                    return;

                case DeactivateNode:
                    w << "DeactivateNode";
                    return;

                case Abort:
                    w << "Abort";
                    return;

                case Restart:
                    w << "Restart";
                    return;

                case Delete:
                    w << "Delete";
                    return;

                case Deactivate:
                    w << "Deactivate";
                    return;

                case ForceAbort:
                    w << "ForceAbort";
                    return;

                case ForceDelete:
                    w << "ForceDelete";
                    return;

                case QueuedDelete:
                    w << "QueuedDelete";
                    return;

                case AppHostDown:
                    w << "AppHostDown";
                    return;

                case Obliterate:
                    w << "Obliterate";
                    return;

                default:
                    Assert::CodingError("Unknown close mode {0}", static_cast<int>(e));
                }
            }

            ENUM_STRUCTURED_TRACE(ReplicaCloseModeName, None, LastValidEnum);
        }
    }
}

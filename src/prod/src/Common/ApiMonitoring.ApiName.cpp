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
        namespace ApiName
        {
            void WriteToTextWriter(TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Invalid:
                    w << "Invalid";
                    break;

                case Open:
                    w << "Open";
                    break;

                case Close:
                    w << "Close";
                    break;

                case Abort:
                    w << "Abort";
                    break;

                case GetNextCopyState:
                    w << "GetNextCopyState";
                    break;

                case GetNextCopyContext:
                    w << "GetNextCopyContext";
                    break;

                case UpdateEpoch:
                    w << "UpdateEpoch";
                    break;

                case OnDataLoss:
                    w << "OnDataLoss";
                    break;

                case ChangeRole:
                    w << "ChangeRole";
                    break;

                case CatchupReplicaSet:
                    w << "CatchupReplicaSet";
                    break;

                case BuildReplica:
                    w << "BuildReplica";
                    break;

                case RemoveReplica:
                    w << "RemoveReplica";
                    break;

                case GetStatus:
                    w << "GetStatus";
                    break;

                case UpdateCatchupConfiguration:
                    w << "UpdateCatchupConfiguration";
                    break;

                case UpdateCurrentConfiguration:
                    w << "UpdateCurrentConfiguration";
                    break;

                case GetQuery:
                    w << "GetQuery";
                    break;

                default:
                    Assert::CodingError("Invalid state for internal enum: {0}", static_cast<int>(e));
                };
            }

            ENUM_STRUCTURED_TRACE(ApiName, Open, LastValidEnum);
        }
    }
}



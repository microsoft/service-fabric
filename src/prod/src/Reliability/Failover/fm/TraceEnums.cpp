// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        namespace PeriodicTaskName
        {
            void WriteToTextWriter(TextWriter & w, Enum const& value)
            {
                switch (value)
                {
                    case BG:
                        w << "BG";
                        return;
                    case CreateContexts:
                        w << "CreateContexts";
                        return;
                    case FTBackgroundManager:
                        w << "FTBackgroundManager";
                        return;
                    case AddThreadContexts:
                        w << "AddThreadContexts";
                        return;
                    case LoadCache:
                        w << "LoadCache";
                        return;
                    case NodeCache:
                        w << "NodeCache";
                        return;
                    case ServiceCache:
                        w << "ServiceCache";
                        return;
                    case ServiceCleanup:
                        w << "ServiceCleanup";
                        return;
                    case AddServiceContexts:
                        w << "AddServiceContexts";
                        return;
                    default:
                        Assert::CodingError("Unknown PeriodicTaskName value: {0}", static_cast<int>(value));
                        return;
                }
            }

            ENUM_STRUCTURED_TRACE(PeriodicTaskName, BG, LastValidEnum)
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        namespace QueueName
        {
            void WriteToTextWriter(TextWriter & w, Enum const& value)
            {
                switch (value)
                {
                    case CommonQueue:
                        w << "CommonQueue";
                        return;
                    case FailoverUnitQueue:
                        w << "FailoverUnitQueue";
                        return;
                    case QueryQueue:
                        w << "QueryQueue";
                        return;
                    default:
                        Assert::CodingError("Unknown QueueName value: {0}", static_cast<int>(value));
                        return;
                }
            }

            ENUM_STRUCTURED_TRACE(QueueName, CommonQueue, LastValidEnum)
        }
    }
}

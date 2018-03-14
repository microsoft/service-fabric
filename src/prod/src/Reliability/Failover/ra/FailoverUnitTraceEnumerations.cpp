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
        namespace FailoverUnitLifeCycleTraceState
        {
            void WriteToTextWriter(TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                case Closed:
                    w << "Closed";
                    break;

                case Opening:
                    w << "Opening";
                    break;

                case Open:
                    w << "Open";
                    break;

                case Available:
                    w << "Available";
                    break;

                case Closing:
                    w << "Closing";
                    break;

                case Down:
                    w << "Down";
                    break;

                default:
                    Assert::CodingError("Unknown value {0}", static_cast<int>(e));
                    break;
                };
            }

            ENUM_STRUCTURED_TRACE(FailoverUnitLifeCycleTraceState, Closed, LastValidEnum);
        }
    }
}

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
        namespace FailoverUnitStates
        {
            void FailoverUnitStates::WriteToTextWriter(TextWriter & w, FailoverUnitStates::Enum const & val)
            {
                switch (val)
                {
                    case Open:
                        w << L"Open"; return;
                    case Closed: 
                        w << L"Closed"; return;
                    default: 
                        Common::Assert::CodingError("Unknown FailoverUnit State {0}", static_cast<int>(val));
                }
            }

            ENUM_STRUCTURED_TRACE(FailoverUnitStates, Open, LastValidEnum);
        }
    }
}

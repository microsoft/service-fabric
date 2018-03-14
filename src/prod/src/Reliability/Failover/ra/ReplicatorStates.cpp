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
        namespace ReplicatorStates
        {

            ENUM_STRUCTURED_TRACE(ReplicatorStates, Opening, LastValidEnum);

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case Opening:
                        w << L"OP"; return;
                    case Opened:
                        w << L"OC"; return;
                    case Closing:
                        w << L"CP"; return;
                    case Closed: 
                        w << L"CC"; return;
                    default: 
                        Assert::CodingError("Unknown Replicator State");
                }
            }
        }
    }
}

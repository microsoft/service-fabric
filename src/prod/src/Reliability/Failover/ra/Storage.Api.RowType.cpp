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
        namespace Storage
        {
            namespace Api
            {
                namespace RowType
                {
                    void RowType::WriteToTextWriter(TextWriter & w, Enum const & val)
                    {
                        switch (val)
                        {
                        case Invalid:
                            w << "Invalid";
                            break;
                        case Test:
                            w << "Test";
                            break;
                        case FailoverUnit:
                            // Do not change - this is persisted in the lfum 
                            w << "FailoverUnitType";
                            break;
                        default:
                            Common::Assert::CodingError("Unknown FailoverUnit Reconfiguration Stage {0}", static_cast<int>(val));
                        }
                    }

                    ENUM_STRUCTURED_TRACE(RowType, Invalid, LastValidEnum);
                }
            }
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace Common;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            namespace ServiceTypeUpdateResult
            {
                void WriteToTextWriter(TextWriter & w, Enum const& value)
                {
                    switch (value)
                    {
                        case Processed:
                            w << "Processed";
                            return;
                        case Stale:
                            w << "Stale";
                            return;
                        case RANotOpen:
                            w << "RA Not Open";
                            return;
                        default:
                            Assert::CodingError("unknown value for enum {0}", static_cast<int>(value));
                            return;
                    }
                }

                ENUM_STRUCTURED_TRACE(ServiceTypeUpdateResult, Processed, LastValidEnum);
            }
        }
    }
}

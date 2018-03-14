// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            namespace Api
            {
                namespace OperationType
                {
                    void WriteToTextWriter(Common::TextWriter & w, Enum const & val)
                    {
                        switch (val)
                        {
                        case Insert:
                            w << "Insert";
                            break;

                        case Delete:
                            w << "Delete";
                            break;

                        case Update:
                            w << "Update";
                            break;

                        default:
                            Assert::CodingError("Unknown enum value {0}", static_cast<int>(val));
                            break;
                        }
                    }
                }
            }
        }
    }
}

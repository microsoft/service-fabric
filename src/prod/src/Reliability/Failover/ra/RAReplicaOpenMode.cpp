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
        namespace RAReplicaOpenMode
        {
            void WriteToTextWriter(TextWriter & w, RAReplicaOpenMode::Enum const & e)
            {
                switch (e)
                {
                case None:
                    w << "N";
                    return;

                case Open:
                    w << "O";
                    return;

                case Reopen:
                    w << "RO";
                    return;

                case ChangeRole:
                    w << "CR";
                    return;

                default:
                    Assert::CodingError("Unknown open mode {0}", static_cast<int>(e));
                }
            }

            ENUM_STRUCTURED_TRACE(RAReplicaOpenMode, None, LastValidEnum);
        }
    }
}

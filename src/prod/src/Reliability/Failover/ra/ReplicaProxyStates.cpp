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
        namespace ReplicaProxyStates
        {
            ENUM_STRUCTURED_TRACE(ReplicaProxyStates, Ready, LastValidEnum);

            void WriteToTextWriter(TextWriter & w, Enum const & val)
            {
                switch (val)
                {
                    case Ready: 
                        w << L"RD"; return;
                    case InBuild: 
                        w << L"IB"; return;
                    case InDrop:
                        w << L"ID"; return;
                    case Dropped:
                        w << L"DD"; return;
                    default: 
                        Assert::CodingError("Unknown Replica Proxy State");
                }
            }
        }
    }
}


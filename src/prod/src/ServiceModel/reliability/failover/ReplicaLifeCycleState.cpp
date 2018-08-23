// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Reliability
{
    namespace ReplicaLifeCycleState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Invalid:
                w << "Invalid";
                break;

            case Closing:
                w << "Closing";
                break;

            default:
                w << "Unknown" << static_cast<int>(e);
                break;
            };
        }

        ENUM_STRUCTURED_TRACE(ReplicaLifeCycleState, Invalid, LastValidEnum);
    }
}

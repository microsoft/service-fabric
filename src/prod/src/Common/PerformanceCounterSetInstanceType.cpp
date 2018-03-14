// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


namespace Common
{
    namespace PerformanceCounterSetInstanceType
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case Single : w << L"Single"; return;
            case Multiple : w << L"Multiple"; return;
            case GlobalAggregate : w << L"GlobalAggregate"; return;
            case GlobalAggregateWithHistory : w << L"GlobalAggregateWithHistory"; return;
            case MultipleAggregate : w << L"MultipleAggregate"; return;
            case InstanceAggregate : w << L"InstanceAggregate"; return;
            }

            w << "PerformanceCounterSetInstanceType(" << static_cast<int>(e) << ')';
        }
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace ServiceModel
{
    namespace Priority
    {
        void WriteToTextWriter(__in TextWriter & w, Priority::Enum val)
        {
            switch (val)
            {
            case NotAssigned: w << "NotAssigned"; return;
            case Low: w << "Low"; return;
            case Normal: w << "Normal"; return;
            case Medium: w << "Medium"; return;
            case High: w << "High"; return;
            case Higher: w << "Higher"; return;
            case Critical: w << "Critical"; return;
            default: w << "Priority(" << static_cast<uint>(val) << ')'; return;
            }
        }

        BEGIN_ENUM_STRUCTURED_TRACE(Priority)

            ADD_CASTED_ENUM_MAP_VALUE_RANGE(Priority, NotAssigned, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE(Priority)
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    namespace AsyncWorkJobItemState
    {
        void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
        {
            switch (e)
            {
            case NotStarted: w << "NotStarted"; return;
            case AsyncPending: w << "AsyncPending"; return;
            case CompletedSync: w << "CompletedSync"; return;
            case CompletedAsync: w << "CompletedAsync"; return;
            default: w << "AsyncWorkJobItemState(" << static_cast<uint>(e) << ')'; return;
            }
        }

        BEGIN_ENUM_STRUCTURED_TRACE(AsyncWorkJobItemState)

        ADD_CASTED_ENUM_MAP_VALUE_RANGE(AsyncWorkJobItemState, NotStarted, LAST_STATE)

        END_ENUM_STRUCTURED_TRACE(AsyncWorkJobItemState)
    }
}

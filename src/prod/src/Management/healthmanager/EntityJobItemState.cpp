// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace HealthManager
    {
        namespace EntityJobItemState 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                case NotStarted: w << "NotStarted"; return;
                case Started: w << "Started"; return;
                case AsyncPending: w << "AsyncPending"; return;
                case Completed: w << "Completed"; return;
                case Canceled: w << "Canceled"; return;
                default: w << "EntityJobItemState(" << static_cast<uint>(e) << ')'; return;
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE( EntityJobItemState )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( EntityJobItemState, NotStarted, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( EntityJobItemState )
        }
    }
}

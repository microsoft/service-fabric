// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        namespace EntityJobItemState 
        {
            enum Enum 
            { 
                NotStarted = 0, 
                Started = 1, 
                AsyncPending = 2, 
                Completed = 3,
                Canceled = 4,

                LAST_STATE = Canceled,
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE( EntityJobItemState )
        }
    }
}

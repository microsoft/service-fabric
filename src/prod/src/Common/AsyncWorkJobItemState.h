// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace AsyncWorkJobItemState
    {
        enum Enum
        {
            NotStarted = 0,
            AsyncPending = 1,
            CompletedSync = 2,
            CompletedAsync = 3,

            LAST_STATE = CompletedAsync,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

        DECLARE_ENUM_STRUCTURED_TRACE(AsyncWorkJobItemState)
    }
}

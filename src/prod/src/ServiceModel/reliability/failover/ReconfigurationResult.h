// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationResult
    {
        enum Enum
        {
            Invalid = 0,
            Completed = 1,
            AbortSwapPrimary = 2,
            ChangeConfiguration = 3,
			Aborted = 4,
            DemoteCompleted = 5,
            LastValidEnum = DemoteCompleted
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);
		
		DECLARE_ENUM_STRUCTURED_TRACE(ReconfigurationResult);
    }
}

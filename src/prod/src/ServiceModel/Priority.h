// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace Priority
    {
        enum Enum
        {
            NotAssigned = 0x0,
            Low = 0x1,
            Normal = 0x10,
            Medium = 0x20,
            High = 0x30,
            Higher = 0x31,
            Critical = 0x40,

            LAST_STATE = Critical,
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum val);
                
        DECLARE_ENUM_STRUCTURED_TRACE(Priority)
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FaultType
    {
        enum Enum
        {
            Invalid = 0,
            Transient = 1,
            Permanent = 2,

            LastValidEnum = Permanent
        };

        Enum FromPublicAPI(::FABRIC_FAULT_TYPE faultType);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(FaultType);
    }
}


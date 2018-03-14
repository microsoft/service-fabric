// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationType
    {
        enum Enum
        {
            Other = 0,
            SwapPrimary = 1,
            Failover = 2,
            None = 3,
            LastValidEnum = None
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const& value);

        ::FABRIC_RECONFIGURATION_TYPE ConvertToPublicReconfigurationType(ReconfigurationType::Enum toConvert);

        DECLARE_ENUM_STRUCTURED_TRACE(ReconfigurationType);
    }
}

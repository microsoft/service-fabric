//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace HealthState
    {
        enum Enum
        {
            Invalid = 0,
            Ok = 0x1,
            Warning = 0x2,
            Error = 0x3,
            Unknown = 0xffff
        };

        std::wstring ToString(Enum const & val);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_HEALTH_STATE ConvertToPublicHealthState(HealthState::Enum toConvert);

        HealthState::Enum ConvertToServiceModelHealthState(FABRIC_HEALTH_STATE toConvert);

        DECLARE_ENUM_STRUCTURED_TRACE(HealthState)
    }
}

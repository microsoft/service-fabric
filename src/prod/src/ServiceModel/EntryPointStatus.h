// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace EntryPointStatus
    {
        enum Enum
        {
            Invalid = 0,
            Pending = 1,
            Starting = 2,
            Started = 3,
            Stopping = 4,
            Stopped = 5,
        };

        std::wstring ToString(Enum const & val);        
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        FABRIC_ENTRY_POINT_STATUS ToPublicApi(Enum const & val);
        Common::ErrorCode FromPublicApi(FABRIC_ENTRY_POINT_STATUS const & publicVal, __out Enum & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Pending)
            ADD_ENUM_VALUE(Starting)
            ADD_ENUM_VALUE(Started)
            ADD_ENUM_VALUE(Stopping)
            ADD_ENUM_VALUE(Stopped)
        END_DECLARE_ENUM_SERIALIZER()
    }
}

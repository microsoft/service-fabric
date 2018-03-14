// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ApplicationTypeStatus
    {
        enum Enum
        {
            Invalid = 0,
            Provisioning = 1,
            Available = 2,
            Unprovisioning = 3,
            Failed = 4,
        };

        std::wstring ToString(Enum const & val);
        
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_APPLICATION_TYPE_STATUS ToPublicApi(ApplicationTypeStatus::Enum toConvert);

        ApplicationTypeStatus::Enum FromPublicApi(FABRIC_APPLICATION_TYPE_STATUS toConvert);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Provisioning)
            ADD_ENUM_VALUE(Available)
            ADD_ENUM_VALUE(Unprovisioning)
            ADD_ENUM_VALUE(Failed)
        END_DECLARE_ENUM_SERIALIZER()
    }
}


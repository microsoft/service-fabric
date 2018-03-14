// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ApplicationStatus
    {
        enum Enum
        {
            Invalid = 0,
            Ready = 1,
            Upgrading = 2,
            Creating = 3,
            Deleting = 4,
            Failed = 5
        };

        std::wstring ToString(Enum const & val);
        
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

        ::FABRIC_APPLICATION_STATUS ConvertToPublicApplicationStatus(ApplicationStatus::Enum toConvert);

        ApplicationStatus::Enum ConvertToServiceModelApplicationStatus(FABRIC_APPLICATION_STATUS toConvert);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Ready)
            ADD_ENUM_VALUE(Upgrading)
            ADD_ENUM_VALUE(Creating)
            ADD_ENUM_VALUE(Deleting)
            ADD_ENUM_VALUE(Failed)
        END_DECLARE_ENUM_SERIALIZER()
    }
}

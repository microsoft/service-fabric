// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace DeploymentStatus
    {
        enum Enum
        {
            Invalid = 0,
            Downloading = 1,
            Activating = 2,
            Active = 3,
            Upgrading = 4,
            Deactivating = 5,
        };

        std::wstring ToString(Enum const & val);        
        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        FABRIC_DEPLOYMENT_STATUS ToPublicApi(Enum const & val);
        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYMENT_STATUS const & publicVal, __out Enum & val);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Downloading)
            ADD_ENUM_VALUE(Activating)
            ADD_ENUM_VALUE(Active)
            ADD_ENUM_VALUE(Upgrading)
            ADD_ENUM_VALUE(Deactivating)
        END_DECLARE_ENUM_SERIALIZER()

    }
}

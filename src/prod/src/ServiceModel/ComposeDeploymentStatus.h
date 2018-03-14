// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ComposeDeploymentStatus
    {
        enum Enum
        {
            Invalid = 0,
            Provisioning = 1,
            Creating = 2,
            Ready = 3,
            Deleting = 4,
            Unprovisioning = 5,
            Failed = 6,
            Upgrading = 7
        };

        FABRIC_COMPOSE_DEPLOYMENT_STATUS ToPublicApi(Enum const &);

        Enum FromPublicApi(__in FABRIC_COMPOSE_DEPLOYMENT_STATUS const & publicComposeStatus);

        std::wstring ToString(Enum const & val);

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Provisioning)
            ADD_ENUM_VALUE(Creating)
            ADD_ENUM_VALUE(Ready)
            ADD_ENUM_VALUE(Deleting)
            ADD_ENUM_VALUE(Unprovisioning)
            ADD_ENUM_VALUE(Failed)
            ADD_ENUM_VALUE(Upgrading)
        END_DECLARE_ENUM_SERIALIZER()
    }
}

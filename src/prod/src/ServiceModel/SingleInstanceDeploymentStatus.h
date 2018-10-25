//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace SingleInstanceDeploymentStatus
    {
        enum Enum
        {
            Invalid = 0,
            Creating = 1,
            Ready = 2,
            Deleting = 3,
            Failed = 4,
            Upgrading = 5
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(Invalid)
            ADD_ENUM_VALUE(Creating)
            ADD_ENUM_VALUE(Ready)
            ADD_ENUM_VALUE(Deleting)
            ADD_ENUM_VALUE(Failed)
            ADD_ENUM_VALUE(Upgrading)
        END_DECLARE_ENUM_SERIALIZER()
    }
}

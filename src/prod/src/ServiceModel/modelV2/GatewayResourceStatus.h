//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        namespace GatewayResourceStatus
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

            std::wstring ToString(Enum const & val);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

            ::FABRIC_GATEWAY_RESOURCE_STATUS ToPublicApi(GatewayResourceStatus::Enum toConvert);

            GatewayResourceStatus::Enum FromPublicApi(::FABRIC_GATEWAY_RESOURCE_STATUS toConvert);

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
}
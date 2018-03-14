// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ClientServerMessageBody
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        virtual ~ClientServerMessageBody() {}

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
        END_JSON_SERIALIZABLE_PROPERTIES()

        virtual Serialization::FabricSerializable const & GetTcpMessageBody()
        {
            return *this;
        }

        virtual Common::IFabricJsonSerializable const & GetHttpMessageBody()
        {
            return *this;
        }
    };
}

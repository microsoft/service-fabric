// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ClientServerTransport
{
    class FabricVersionMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:
        FabricVersionMessageBody() {}
        FabricVersionMessageBody(Common::FabricVersion version) : version_(version) {}

        __declspec(property(get=get_Version)) Common::FabricVersion const & Version;
        Common::FabricVersion const & get_Version() const { return version_; }

        virtual Serialization::FabricSerializable& GetTcpMessageBody()
        {
            return version_;
        }

    private:
        Common::FabricVersion version_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class ClientDataBody : public Serialization::FabricSerializable
    {
    public:
        ClientDataBody()
        {
        }

        ClientDataBody(std::vector<byte> && clientData)
            : clientData_(move(clientData))
        {
        }

        __declspec(property(get=get_ClientData)) std::vector<byte> & ClientData;
        std::vector<byte> & get_ClientData() { return clientData_; }

        FABRIC_FIELDS_01(clientData_);

    private:
        std::vector<byte> clientData_;
    };
};

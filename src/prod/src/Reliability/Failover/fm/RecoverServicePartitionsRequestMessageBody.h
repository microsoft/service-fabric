// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class RecoverServicePartitionsRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        RecoverServicePartitionsRequestMessageBody()
        {
        }

        RecoverServicePartitionsRequestMessageBody(std::wstring const& serviceName)
            : serviceName_(serviceName)
        {
        }

        __declspec (property(get=get_ServiceName)) std::wstring const& ServiceName;
        std::wstring const& get_ServiceName() const { return serviceName_; }

        FABRIC_FIELDS_01(serviceName_);

    private:

        std::wstring serviceName_;
    };
}

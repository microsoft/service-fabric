// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ClientCommandReply : public Serialization::FabricSerializable
    {
    public:
        ClientCommandReply(
            std::wstring const& reply, 
            Common::ErrorCodeValue::Enum errorCodeValue,
            Transport::MessageId const& messageId)
            : reply_(reply),
            errorCodeValue_(errorCodeValue),
            messageId_(messageId)
        {
        }

        ClientCommandReply() {}

        __declspec (property(get=get_ErrorCodeValue)) Common::ErrorCodeValue::Enum ErrorCodeValue;
        Common::ErrorCodeValue::Enum get_ErrorCodeValue() const { return errorCodeValue_; }

        __declspec (property(get=get_Reply)) std::wstring const& Reply;
        std::wstring const& get_Reply() const { return reply_; }

        __declspec (property(get=get_MessageId)) Transport::MessageId const& MessageId;
        Transport::MessageId const& get_MessageId() const { return messageId_; }

        FABRIC_FIELDS_03(reply_, errorCodeValue_, messageId_);

    private:
        std::wstring reply_;
        Common::ErrorCodeValue::Enum errorCodeValue_;
        Transport::MessageId messageId_;
    };
};

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class ErrorCodeMessageBody : public Common::IFabricJsonSerializable
    {
    public:
        ErrorCodeMessageBody()
        {
        }

        ErrorCodeMessageBody(std::wstring & errorCode, std::wstring & errorMessage)
            : errorCode_(errorCode), errorMessage_(errorMessage)
        {
        }

        std::wstring GetErrorCode()
        {
            return errorCode_;
        }

        void SetErrorCode(const std::wstring & errorCode)
        {
            errorCode_ = errorCode;
        }

        std::wstring GetErrorMessage()
        {
            return errorMessage_;
        }

        void SetErrorMessage(const std::wstring & errorMessage)
        {
            errorMessage_ = errorMessage;
        }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(::ServiceModel::Constants::Code, errorCode_)
            SERIALIZABLE_PROPERTY(::ServiceModel::Constants::Message, errorMessage_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring errorCode_;
        std::wstring errorMessage_;
    };
}

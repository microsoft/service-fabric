// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace HttpGateway
{
    class ErrorBody : public Common::IFabricJsonSerializable
    {
    public:
        ErrorBody(HttpGateway::ErrorCodeMessageBody errorCodeMessageBody);
        static ErrorBody CreateErrorBodyFromErrorCode(Common::ErrorCode errorCode);
        std::wstring GetErrorCode();
        std::wstring GetErrorMessage();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(::ServiceModel::Constants::Error, errorCodeMessageBody_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        static std::wstring FromErrorCode(Common::ErrorCode errorCode);
        static std::wstring FromErrorMessage(Common::ErrorCode errorCode);
        HttpGateway::ErrorCodeMessageBody errorCodeMessageBody_;
    };
}

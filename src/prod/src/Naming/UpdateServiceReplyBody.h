// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class UpdateServiceReplyBody : public Serialization::FabricSerializable
    {
    public:
        UpdateServiceReplyBody()
            : validationError_(Common::ErrorCodeValue::Success) 
            , msg_()
        {
        }

        UpdateServiceReplyBody(
            Common::ErrorCodeValue::Enum error,
            std::wstring && msg)
            : validationError_(error)
            , msg_(move(msg))
        {
        }

        __declspec(property(get=get_ValidationError)) Common::ErrorCodeValue::Enum ValidationError;
        Common::ErrorCodeValue::Enum get_ValidationError() const { return validationError_; }

        std::wstring && TakeErrorMessage() { return std::move(msg_); }

        FABRIC_FIELDS_02(validationError_, msg_);

    private:
        Common::ErrorCodeValue::Enum validationError_;
        std::wstring msg_;
    };
}

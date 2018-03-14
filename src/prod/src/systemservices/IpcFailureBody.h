// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class IpcFailureBody : public Serialization::FabricSerializable
    {
    public:
        IpcFailureBody() { }

        explicit IpcFailureBody(Common::ErrorCodeValue::Enum const & error) : error_(error) { }

        explicit IpcFailureBody(Common::ErrorCode const & error)
            : error_(error.ReadValue())
            , message_(error.Message)
        {
        }

        __declspec (property(get=get_Error)) Common::ErrorCodeValue::Enum Error;
        Common::ErrorCodeValue::Enum get_Error() const { return error_; }

        Common::ErrorCode TakeError() { return Common::ErrorCode(error_, std::move(message_)); }

        FABRIC_FIELDS_02(error_, message_);

    private:
        Common::ErrorCodeValue::Enum error_;
        std::wstring message_;
    };
}


// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ClientOperationFailureMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        ClientOperationFailureMessageBody()
            : value_(Common::ErrorCodeValue::Success)
            , message_()
        {
        }

        explicit ClientOperationFailureMessageBody(Common::ErrorCode && error)
            : value_(error.ReadValue())
            , message_(error.TakeMessage())
        {
        }

        explicit ClientOperationFailureMessageBody(Common::ErrorCode & error)
            : value_(error.ReadValue())
            , message_(error.TakeMessage())
        {
        }

        Common::ErrorCode TakeError()
        {
            return Common::ErrorCode(value_, std::move(message_));
        }

        FABRIC_FIELDS_02(value_, message_);

    private:
        // Store ErrorCode fields individually for backwards compatibility.
        // Prior versions of this class only contained ErrorCodeValue::Enum.
        //
        Common::ErrorCodeValue::Enum value_;
        std::wstring message_;
    };
}

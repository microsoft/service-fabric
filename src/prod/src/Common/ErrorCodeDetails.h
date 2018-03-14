// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ErrorCodeDetails : public Serialization::FabricSerializable
    {
    public:
        ErrorCodeDetails();
        ErrorCodeDetails(ErrorCode const&);
        ErrorCodeDetails(ErrorCode &&);

        bool HasErrorMessage() const;
        ErrorCode TakeError();

        FABRIC_FIELDS_02(value_, message_);

    private:
        ErrorCodeValue::Enum value_;
        std::wstring message_;
    };
}

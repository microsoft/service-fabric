// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ContainerOperationReply : public Serialization::FabricSerializable
    {
    public:
        ContainerOperationReply();
        ContainerOperationReply(
            Common::ErrorCode const & error,
            std::wstring const & errorMessage);

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_ErrorMessage)) std::wstring const & ErrorMessage;
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }

        std::wstring && TakeErrorMessage();

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(error_, errorMessage_);

    private:
        Common::ErrorCode error_;
        std::wstring errorMessage_;
    };
}

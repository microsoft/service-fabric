// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricHostOperationReply : public Serialization::FabricSerializable
    {
    public:
        FabricHostOperationReply();
        FabricHostOperationReply(
            Common::ErrorCode const & error,
            std::wstring const & errorMessage = std::wstring(),
            std::wstring const & data = std::wstring());
            
        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_ErrorMessage)) std::wstring const & ErrorMessage;
        std::wstring const & get_ErrorMessage() const { return errorMessage_; }

        __declspec(property(get = get_Data)) std::wstring const & Data;
        std::wstring const & get_Data() const { return data_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(error_, errorMessage_,data_);

    private:
        Common::ErrorCode error_;
        std::wstring errorMessage_;
        std::wstring data_;
    };
}

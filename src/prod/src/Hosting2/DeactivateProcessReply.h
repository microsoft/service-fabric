// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class DeactivateProcessReply : public Serialization::FabricSerializable
    {
    public:
        DeactivateProcessReply();
        DeactivateProcessReply(
            std::wstring const & applicationServiceId,
            Common::ErrorCode error);
            

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get=get_ApplicationServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_ApplicationServiceId() const { return appServiceId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(appServiceId_, error_);

    private:
        std::wstring appServiceId_;
        Common::ErrorCode error_;
    };
}

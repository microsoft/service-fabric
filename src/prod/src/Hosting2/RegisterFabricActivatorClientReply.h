// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class RegisterFabricActivatorClientReply : public Serialization::FabricSerializable
    {
    public:
        RegisterFabricActivatorClientReply();
        RegisterFabricActivatorClientReply(
            Common::ErrorCode error);
            
        __declspec(property(get=get_Error)) Common::ErrorCode Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(error_);

    private:
        Common::ErrorCode error_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class EnableNodeRequest : public Serialization::FabricSerializable
    {
    public:
        EnableNodeRequest();
        EnableNodeRequest(std::wstring const & reason);


        __declspec(property(get=get_DisableReason)) std::wstring const & DisableReason;
        std::wstring const & get_DisableReason() const { return disableReason_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(disableReason_);

    private:
        std::wstring disableReason_;
    };
}

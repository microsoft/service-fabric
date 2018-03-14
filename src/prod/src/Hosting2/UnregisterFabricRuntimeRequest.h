// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class UnregisterFabricRuntimeRequest : public Serialization::FabricSerializable
    {
    public:
        UnregisterFabricRuntimeRequest();
        UnregisterFabricRuntimeRequest(std::wstring const & runtimeId);

        __declspec(property(get=get_hostId)) std::wstring const & Id;
        std::wstring const & get_hostId() const { return id_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(id_);

    private:
        std::wstring id_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetFabricProcessSidReply : public Serialization::FabricSerializable
    {
    public:
        GetFabricProcessSidReply();
        GetFabricProcessSidReply(std::wstring const & fabricProcessSid, DWORD fabricProcessId);

        __declspec(property(get=get_FabricProcessSid)) std::wstring const & FabricProcessSid;
        std::wstring const & get_FabricProcessSid() const { return fabricProcessSid_; }

        __declspec(property(get=get_FabricProcessId)) DWORD FabricProcessId;
        DWORD get_FabricProcessId() const { return fabricProcessId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_02(fabricProcessSid_, fabricProcessId_);

    private:
        std::wstring fabricProcessSid_;
        DWORD fabricProcessId_;
    };
}

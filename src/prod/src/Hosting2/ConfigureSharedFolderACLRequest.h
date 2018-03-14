// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class ConfigreSharedFolderACLRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigreSharedFolderACLRequest();
        ConfigreSharedFolderACLRequest(
            std::vector<std::wstring> const & sharedFolders,
            int64 timeoutTicks);

        __declspec(property(get=get_SharedFolders)) std::vector<std::wstring> const & SharedFolders;
        std::vector<std::wstring> const & get_SharedFolders() const { return sharedFolders_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return ticks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(sharedFolders_, ticks_);

    private:
        std::vector<std::wstring> sharedFolders_;
        int64 ticks_;
    };
}

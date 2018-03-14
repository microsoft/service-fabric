// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DeleteApplicationFoldersReply : public Serialization::FabricSerializable
    {
    public:
        DeleteApplicationFoldersReply();
        DeleteApplicationFoldersReply(
            Common::ErrorCode const & error,
            std::vector<std::wstring> const & deletedAppFolders);
            
        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_DeletedAppFolders)) std::vector<std::wstring> const & DeletedAppFolders;
        std::vector<std::wstring> const & get_DeletedAppFolders() const { return deletedAppFolders_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(error_, deletedAppFolders_);

    private:
        Common::ErrorCode error_;
        std::vector<std::wstring> deletedAppFolders_;
    };
}

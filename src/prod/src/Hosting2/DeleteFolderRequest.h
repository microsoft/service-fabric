// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DeleteFolderRequest : public Serialization::FabricSerializable
    {
    public:
        DeleteFolderRequest();
        DeleteFolderRequest(
            std::wstring const & nodeId,
            std::vector<std::wstring> const & applicationFolders);

        __declspec(property(get=get_ApplicationFolders)) std::vector<std::wstring> const & ApplicationFolders;
        std::vector<std::wstring> const & get_ApplicationFolders() const { return appFolders_; }

         __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(nodeId_, appFolders_);


    private:
        std::wstring nodeId_;
        std::vector<std::wstring> appFolders_;
    };
}

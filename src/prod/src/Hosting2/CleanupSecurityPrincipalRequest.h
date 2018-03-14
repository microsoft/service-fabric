// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CleanupSecurityPrincipalRequest : public Serialization::FabricSerializable
    {
    public:
        CleanupSecurityPrincipalRequest();
        CleanupSecurityPrincipalRequest(
            std::wstring const & nodeId,
            std::vector<std::wstring> const & applicationIds,
            bool const cleanupForNode);

        __declspec(property(get=get_ApplicationIds)) std::vector<std::wstring> const & ApplicationIds;
        std::vector<std::wstring> const & get_ApplicationIds() const { return applicationIds_; }

         __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_IsCleanupForNode)) bool const  IsCleanupForNode;
        bool const get_IsCleanupForNode() const { return cleanupForNode_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(nodeId_, applicationIds_, cleanupForNode_);


    private:
        std::wstring nodeId_;
        std::vector<std::wstring> applicationIds_;
        bool cleanupForNode_;
    };
}

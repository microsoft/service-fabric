// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ManageOverlayNetworkResourcesReply : public Serialization::FabricSerializable
    {
    public:
        ManageOverlayNetworkResourcesReply();

        ManageOverlayNetworkResourcesReply(
            ManageOverlayNetworkAction::Enum action,
            Common::ErrorCode const & error,
            std::map<std::wstring, std::map<std::wstring, std::wstring>> const & assignedNetworkResources);

        ~ManageOverlayNetworkResourcesReply();

        __declspec(property(get = get_Action)) ManageOverlayNetworkAction::Enum Action;
        ManageOverlayNetworkAction::Enum get_Action() const { return action_; }

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get=get_AssignedNetworkResources)) std::map<std::wstring, std::wstring> const & AssignedNetworkResources;
        std::map<std::wstring, std::wstring> const & get_AssignedNetworkResources() const { return assignedNetworkResources_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(action_, error_, assignedNetworkResources_);

    private:
        ManageOverlayNetworkAction::Enum action_;
        Common::ErrorCode error_;
        std::map<std::wstring, std::wstring> assignedNetworkResources_;
    };
}
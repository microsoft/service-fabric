// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeleteApplicationRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        DeleteApplicationRequestMessageBody() { }

        DeleteApplicationRequestMessageBody(
            ServiceModel::ApplicationIdentifier const & appId,
            uint64 instanceId)
            : appId_(appId)
            , instanceId_(instanceId)
        {
        }

        __declspec (property(get=get_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return appId_; }

        __declspec (property(get=get_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w << appId_ << " (" << instanceId_ << ")";
        }

        FABRIC_FIELDS_02(appId_, instanceId_);

    private:
        ServiceModel::ApplicationIdentifier appId_;
        uint64 instanceId_;
    };
}

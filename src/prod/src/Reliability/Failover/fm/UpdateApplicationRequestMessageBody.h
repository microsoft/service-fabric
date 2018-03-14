// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Used for creating or updating scaleout parameters of an application.
    class UpdateApplicationRequestMessageBody : public Serialization::FabricSerializable
    {
    public:
        UpdateApplicationRequestMessageBody() {}

        UpdateApplicationRequestMessageBody(
            Common::NamingUri const & appName,
            ServiceModel::ApplicationIdentifier const & applicationId,
            uint64 instanceId,
            uint64 updateId,
            ApplicationCapacityDescription const & appCapacity)
            : applicationName_(appName)
            , applicationId_(applicationId)
            , instanceId_(instanceId)
            , updateId_(updateId)
            , capacityDescription_(appCapacity)
        {}

        __declspec(property(get = get_ApplicationName, put = put_ApplicationName)) Common::NamingUri & ApplicationName;
        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
        void put_ApplicationName(Common::NamingUri const & applicationName) { applicationName_ = applicationName; }

        __declspec(property(get = get_ApplicationId, put = put_ApplicationId)) ServiceModel::ApplicationIdentifier & ApplicationId;
        ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return applicationId_; }
        void put_ApplicationId(ServiceModel::ApplicationIdentifier const & applicationId) { applicationId_ = applicationId; }

        __declspec(property(get = get_InstanceId, put = put_InstanceId)) uint64 InstanceId;
        uint64 get_InstanceId() const { return instanceId_; }
        void put_InstanceId(uint64 instanceId) { instanceId_ = instanceId; }

        __declspec(property(get = get_UpdateId, put = put_UpdateId)) uint64 UpdateId;
        uint64 get_UpdateId() const { return updateId_; }
        void put_UpdateId(uint64 updateId) { updateId_ = updateId; }

        __declspec(property(get = get_ApplicationCapacity, put = put_ApplicationCapacity)) ApplicationCapacityDescription const & ApplicationCapacity;
        ApplicationCapacityDescription const & get_ApplicationCapacity() const { return capacityDescription_; }
        void put_ApplicationCapacity(ApplicationCapacityDescription const& appCapacity) { capacityDescription_ = appCapacity; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.WriteLine("Name={0} InstanceId={1} UpdateId={2} ApplicationCapacity={3}",
                applicationName_,
                instanceId_,
                updateId_,
                capacityDescription_);
        }

        FABRIC_FIELDS_05(applicationName_, applicationId_, instanceId_, updateId_, capacityDescription_);

    private:
        Common::NamingUri applicationName_;
        ServiceModel::ApplicationIdentifier applicationId_;
        uint64 instanceId_;
        uint64 updateId_;
        ApplicationCapacityDescription capacityDescription_;
    };
}

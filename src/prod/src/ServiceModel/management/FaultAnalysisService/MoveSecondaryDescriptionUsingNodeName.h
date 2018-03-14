// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class MoveSecondaryDescriptionUsingNodeName
            : public Serialization::FabricSerializable
        {
        public:

            MoveSecondaryDescriptionUsingNodeName();
            MoveSecondaryDescriptionUsingNodeName(MoveSecondaryDescriptionUsingNodeName const &);
            MoveSecondaryDescriptionUsingNodeName(MoveSecondaryDescriptionUsingNodeName &&);

            __declspec(property(get = get_CurrentSecondaryNodeName)) std::wstring const & CurrentSecondaryNodeName;
            __declspec(property(get = get_NewSecondaryNodeName)) std::wstring const & NewSecondaryNodeName;
            __declspec(property(get = get_ServiceName)) Common::NamingUri const & ServiceName;
            __declspec(property(get = get_PartitionId)) FABRIC_PARTITION_ID PartitionId;
            __declspec (property(get = get_IgnoreConstraints)) bool const & IgnoreConstraints;

            std::wstring const & get_CurrentSecondaryNodeName() const { return currentSecondaryNodeName_; }
            std::wstring const & get_NewSecondaryNodeName() const { return newSecondaryNodeName_; }
            Common::NamingUri const & get_ServiceName() const { return serviceName_; }
            FABRIC_PARTITION_ID get_PartitionId() const { return partitionId_; }
            bool const & get_IgnoreConstraints() const { return this->ignoreConstraints_; }

            Common::ErrorCode FromPublicApi(FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_MOVE_SECONDARY_DESCRIPTION_USING_NODE_NAME &) const;

            FABRIC_FIELDS_05(
                currentSecondaryNodeName_,
                newSecondaryNodeName_,
                serviceName_,
                partitionId_,
                ignoreConstraints_);

        private:

            std::wstring currentSecondaryNodeName_;
            std::wstring newSecondaryNodeName_;
            Common::NamingUri serviceName_;
            FABRIC_PARTITION_ID partitionId_;
            bool ignoreConstraints_;
        };
    }
}

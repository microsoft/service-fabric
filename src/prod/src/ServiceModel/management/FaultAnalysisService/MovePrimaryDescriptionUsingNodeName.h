// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class MovePrimaryDescriptionUsingNodeName
            : public Serialization::FabricSerializable
        {
        public:

            MovePrimaryDescriptionUsingNodeName();
            MovePrimaryDescriptionUsingNodeName(MovePrimaryDescriptionUsingNodeName const &);
            MovePrimaryDescriptionUsingNodeName(MovePrimaryDescriptionUsingNodeName &&);

            __declspec(property(get = get_NodeName)) std::wstring const & NewPrimaryNodeName;
            __declspec(property(get = get_ServiceName)) Common::NamingUri const & ServiceName;
            __declspec(property(get = get_PartitionId)) FABRIC_PARTITION_ID PartitionId;
            __declspec (property(get = get_IgnoreConstraints)) bool const & IgnoreConstraints;

            std::wstring const & get_NodeName() const { return newPrimaryNodeName_; }
            Common::NamingUri const & get_ServiceName() const { return serviceName_; }
            FABRIC_PARTITION_ID get_PartitionId() const { return partitionId_; }
            bool const & get_IgnoreConstraints() const { return this->ignoreConstraints_; }

            Common::ErrorCode FromPublicApi(FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME const &);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_MOVE_PRIMARY_DESCRIPTION_USING_NODE_NAME &) const;

            FABRIC_FIELDS_04(
                newPrimaryNodeName_,
                serviceName_,
                partitionId_,
                ignoreConstraints_);

        private:

            std::wstring newPrimaryNodeName_;
            Common::NamingUri serviceName_;
            FABRIC_PARTITION_ID partitionId_;
            bool ignoreConstraints_;
        };
    }
}

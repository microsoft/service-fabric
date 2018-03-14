// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class PrimaryMoveResult : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(PrimaryMoveResult)

        public:

            PrimaryMoveResult();
            PrimaryMoveResult(std::wstring const & nodeName, Common::NamingUri const & serviceName, FABRIC_PARTITION_ID const partitionId)
                : nodeName_(nodeName)
                , serviceName_(serviceName)
                , partitionId_(partitionId)
            {};

            PrimaryMoveResult(PrimaryMoveResult &&);

            __declspec(property(get = get_NodeName)) std::wstring const& NodeName;
            std::wstring const& get_NodeName() const { return nodeName_; }

            __declspec(property(get = get_ServiceName)) Common::NamingUri const& ServiceName;
            Common::NamingUri const& get_ServiceName() const { return serviceName_; }

            __declspec(property(get = get_PartitionId)) FABRIC_PARTITION_ID const PartitionId;
            FABRIC_PARTITION_ID const get_PartitionId() const { return partitionId_; }

            Common::ErrorCode FromPublicApi(FABRIC_MOVE_PRIMARY_RESULT const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_MOVE_PRIMARY_RESULT &) const;

            FABRIC_FIELDS_03(
                nodeName_,
                serviceName_,
                partitionId_);

        private:
            std::wstring nodeName_;
            Common::NamingUri serviceName_;
            FABRIC_PARTITION_ID partitionId_;
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class SecondaryMoveResult : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(SecondaryMoveResult)

        public:

            SecondaryMoveResult();
            SecondaryMoveResult(std::wstring const & currentSecondaryNodeName, std::wstring const & newSecondaryNodeName, Common::NamingUri const & serviceName, FABRIC_PARTITION_ID const partitionId)
                : currentSecondaryNodeName_(currentSecondaryNodeName)
                , newSecondaryNodeName_(newSecondaryNodeName)
                , serviceName_(serviceName)
                , partitionId_(partitionId)
            {};

            SecondaryMoveResult(SecondaryMoveResult &&);

            __declspec(property(get = get_CurrentSecondaryNodeName)) std::wstring const& CurrentSecondaryNodeName;
            std::wstring const& get_CurrentSecondaryNodeName() const { return currentSecondaryNodeName_; }

            __declspec(property(get = get_NewSecondaryNodeName)) std::wstring const& NewSecondaryNodeName;
            std::wstring const& get_NewSecondaryNodeName() const { return newSecondaryNodeName_; }

            __declspec(property(get = get_ServiceName)) Common::NamingUri const& ServiceName;
            Common::NamingUri const& get_ServiceName() const { return serviceName_; }

            __declspec(property(get = get_PartitionId)) FABRIC_PARTITION_ID const PartitionId;
            FABRIC_PARTITION_ID const get_PartitionId() const { return partitionId_; }

            Common::ErrorCode FromPublicApi(FABRIC_MOVE_SECONDARY_RESULT const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_MOVE_SECONDARY_RESULT &) const;

            FABRIC_FIELDS_04(
                currentSecondaryNodeName_,
                newSecondaryNodeName_,
                serviceName_,
                partitionId_);

        private:
            std::wstring currentSecondaryNodeName_;
            std::wstring newSecondaryNodeName_;
            Common::NamingUri serviceName_;
            FABRIC_PARTITION_ID partitionId_;
        };
    }
}

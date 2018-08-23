// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        class MonitoringData
        {
        DEFAULT_COPY_ASSIGNMENT(MonitoringData)
        DEFAULT_COPY_CONSTRUCTOR(MonitoringData)

        public:
            MonitoringData(
                Common::Guid const & partitionId,
                int64 replicaId,
                int64 replicaInstanceId,
                ApiNameDescription apiName,
                Common::StopwatchTime startTime) :
                partitionId_(partitionId),
                replicaId_(replicaId),
                replicaInstanceId_(replicaInstanceId),
                apiName_(std::move(apiName)),
                startTime_(startTime)
            {
            }

            MonitoringData(MonitoringData && other) :
                partitionId_(std::move(other.partitionId_)),
                replicaId_(std::move(other.replicaId_)),
                replicaInstanceId_(std::move(other.replicaInstanceId_)),
                apiName_(std::move(other.apiName_)),
                startTime_(std::move(other.startTime_)),
                serviceType_(std::move(other.serviceType_))
            {
            }

            MonitoringData & operator=(MonitoringData && other)                
            {
                if (this != &other)
                {
                    partitionId_ = std::move(other.partitionId_);
                    replicaId_ = std::move(other.replicaId_);
                    replicaInstanceId_ = std::move(other.replicaInstanceId_);
                    apiName_ = std::move(other.apiName_);
                    startTime_ = std::move(other.startTime_);
					serviceType_ = std::move(other.serviceType_);
                }

                return *this;
            }

            __declspec(property(get = get_PartitionId)) Common::Guid const & PartitionId;
            Common::Guid const & get_PartitionId() const { return partitionId_; }

            __declspec(property(get = get_ReplicaId)) int64 ReplicaId;
            int64 get_ReplicaId() const { return replicaId_; }

            __declspec(property(get = get_ReplicaInstanceId)) int64 ReplicaInstanceId;
            int64 get_ReplicaInstanceId() const { return replicaInstanceId_; }

            __declspec(property(get = get_Api)) ApiNameDescription const & Api;
            ApiNameDescription const & get_Api() const { return apiName_; }

            __declspec(property(get = get_StartTime)) Common::StopwatchTime StartTime;
            Common::StopwatchTime get_StartTime() const { return startTime_; }

            __declspec(property(get = get_ServiceType, put=set_ServiceType)) std::wstring const & ServiceType;
            std::wstring const & get_ServiceType() const { return serviceType_; }

            void set_ServiceType(std::wstring const & value) { serviceType_ = value; }

        private:
            Common::Guid partitionId_;
            int64 replicaId_;
            int64 replicaInstanceId_;
            Common::StopwatchTime startTime_;
            ApiNameDescription apiName_;

            // Todo: avoid copying strings
            std::wstring serviceType_;
        };
    }
}



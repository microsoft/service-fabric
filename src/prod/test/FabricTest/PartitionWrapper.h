// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    inline void AssertGetStatusReturnValue(HRESULT hr)
    {
        ASSERT_IF(FAILED(hr) && hr != FABRIC_E_OBJECT_CLOSED, "Incorrect rv from read/writestatus");
    }

    template<typename T>
    struct PartitionTraits
    {
    };

    template<>
    struct PartitionTraits<IFabricStatefulServicePartition>
    {
        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus(IFabricStatefulServicePartition * partition)
        {
            if (partition == nullptr)
            {
                return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
            }

            FABRIC_SERVICE_PARTITION_ACCESS_STATUS status = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
            HRESULT hr = partition->GetWriteStatus(&status);
            AssertGetStatusReturnValue(hr);
            return status;
        }

        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus(IFabricStatefulServicePartition * partition)
        {
            if (partition == nullptr)
            {
                return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
            }

            FABRIC_SERVICE_PARTITION_ACCESS_STATUS status = FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
            auto hr = partition->GetReadStatus(&status);
            AssertGetStatusReturnValue(hr);
            return status;
        }
    };

    template<>
    struct PartitionTraits<IFabricStatefulServicePartition1>
    {
        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus(IFabricStatefulServicePartition *partition)
        {
            return PartitionTraits<IFabricStatefulServicePartition>::GetWriteStatus(partition);
        }

        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus(IFabricStatefulServicePartition *partition)
        {
            return PartitionTraits<IFabricStatefulServicePartition>::GetReadStatus(partition);
        }
    };

    template<>
    struct PartitionTraits<IFabricStatefulServicePartition3>
    {
        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus(IFabricStatefulServicePartition1 *partition)
        {
            return PartitionTraits<IFabricStatefulServicePartition1>::GetWriteStatus(partition);
        }

        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus(IFabricStatefulServicePartition1 *partition)
        {
            return PartitionTraits<IFabricStatefulServicePartition1>::GetReadStatus(partition);
        }
    };

    template<>
    struct PartitionTraits<IFabricStatelessServicePartition1>
    {
        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus(IFabricStatelessServicePartition *)
        {
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
        }

        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus(IFabricStatelessServicePartition *)
        {
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
        }
    };

    template<>
    struct PartitionTraits<IFabricStatelessServicePartition3>
    {
        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus(IFabricStatelessServicePartition *)
        {
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
        }

        static FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus(IFabricStatelessServicePartition *)
        {
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_INVALID;
        }
    };

    template<typename T>
    class PartitionWrapper
    {
    public:

        void ReportFault(::FABRIC_FAULT_TYPE faultType) const
        {
            if (partition_.GetRawPointer() != nullptr)
            {
                partition_->ReportFault(faultType);
            }
        }

        FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatus() const
        {
            return PartitionTraits<T>::GetWriteStatus(partition_.GetRawPointer());
        }

        FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatus() const
        {
            return PartitionTraits<T>::GetReadStatus(partition_.GetRawPointer());
        }

        void ReportLoad(ULONG metricCount, ::FABRIC_LOAD_METRIC const *metrics) const
        {
            if (partition_.GetRawPointer() != nullptr)
            {
                partition_->ReportLoad(metricCount, metrics);
            }
        }

        Common::ErrorCode ReportPartitionHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            if (partition_.GetRawPointer() != nullptr)
            {
                HRESULT result;
                if (sendOptions == nullptr)
                {
                    result = partition_->ReportPartitionHealth(healthInfo);
                }
                else
                {
                    result = partition_->ReportPartitionHealth2(healthInfo, sendOptions);
                }

                return Common::ErrorCode::FromHResult(result);
            }

            return Common::ErrorCode(Common::ErrorCodeValue::PartitionNotFound, L"Partition object was closed");
        }

        void ReportMoveCost(::FABRIC_MOVE_COST moveCost) const
        {
            if (partition_.GetRawPointer() != nullptr)
            {
                partition_->ReportMoveCost(moveCost);
            }
        }

    protected:
        Common::ComPointer<T> partition_;
    };

    template<typename T>
    class StatefulPartitionWrapper : public PartitionWrapper<T>
    {
    
    public :
        void SetPartition(Common::ComPointer<T> const & partition)
        {
            this->partition_ = partition;
        }

        Common::ErrorCode ReportReplicaHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            if (this->partition_.GetRawPointer() != nullptr)
            {
                HRESULT result;
                if (sendOptions == nullptr)
                {
                    result = this->partition_->ReportReplicaHealth(healthInfo);
                }
                else
                {
                    result = this->partition_->ReportReplicaHealth2(healthInfo, sendOptions);
                }

                return Common::ErrorCode::FromHResult(result);
            }

            return Common::ErrorCode(Common::ErrorCodeValue::PartitionNotFound, L"Partition object was closed");
        }
    };

    template<typename T>
    class StatelessPartitionWrapper : public PartitionWrapper<T>
    {
    
    public:
        void SetPartition(Common::ComPointer<T> const & partition)
        {
            this->partition_ = partition;
        }

        Common::ErrorCode ReportInstanceHealth(
            ::FABRIC_HEALTH_INFORMATION const *healthInfo,
            ::FABRIC_HEALTH_REPORT_SEND_OPTIONS const * sendOptions) const
        {
            if (this->partition_.GetRawPointer() != nullptr)
            {
                HRESULT result;
                if (sendOptions == nullptr)
                {
                    result = this->partition_->ReportInstanceHealth(healthInfo);
                }
                else
                {
                    result = this->partition_->ReportInstanceHealth2(healthInfo, sendOptions);
                }

                return Common::ErrorCode::FromHResult(result);
            }

            return Common::ErrorCode(Common::ErrorCodeValue::PartitionNotFound, L"Partition object was closed");
        }
    };
}

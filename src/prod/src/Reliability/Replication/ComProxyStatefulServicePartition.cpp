// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"
#include "ComProxyStatefulServicePartition.h"

namespace Reliability
{
    namespace ReplicationComponent
    {
        using Common::Assert;

        ComProxyStatefulServicePartition::ComProxyStatefulServicePartition()
            : servicePartition_()
        {
        }

        ComProxyStatefulServicePartition::ComProxyStatefulServicePartition(
            Common::ComPointer<IFabricStatefulServicePartition> && servicePartition)
            : servicePartition_(std::move(servicePartition))
        {
        }

        ComProxyStatefulServicePartition::ComProxyStatefulServicePartition(
            ComProxyStatefulServicePartition const & other)
            : servicePartition_(other.servicePartition_)
        {
        }

        ComProxyStatefulServicePartition::ComProxyStatefulServicePartition(
            ComProxyStatefulServicePartition && other)
            : servicePartition_(std::move(other.servicePartition_))
        {
        }

        ComProxyStatefulServicePartition & ComProxyStatefulServicePartition::operator=(
            ComProxyStatefulServicePartition && other)
        {
            if(this != &other)
            {
                servicePartition_ = std::move(other.servicePartition_);
            }

            return *this;
        }

        Common::ErrorCode ComProxyStatefulServicePartition::GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const ** partitionInfo) const
        {
            HRESULT result = servicePartition_->GetPartitionInfo(partitionInfo);

            return Common::ErrorCode::FromHResult(result);
        }

        FABRIC_SERVICE_PARTITION_ACCESS_STATUS ComProxyStatefulServicePartition::GetReadStatus() const
        {
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus;
            HRESULT result = servicePartition_->GetReadStatus(&readStatus);
            if(result == S_OK)
            {
                return readStatus;
            }
                
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
        }

        FABRIC_SERVICE_PARTITION_ACCESS_STATUS ComProxyStatefulServicePartition::GetWriteStatus() const
        {
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus;
            HRESULT result = servicePartition_->GetWriteStatus(&writeStatus);
            if(result == S_OK)
            {
                return writeStatus;
            }
                
            return FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY;
        }

        Common::ErrorCode ComProxyStatefulServicePartition::ReportFault(::FABRIC_FAULT_TYPE faultType) const
        {
            if (servicePartition_)
            {
                return Common::ErrorCode::FromHResult(servicePartition_->ReportFault(faultType));
            }
            else
            {
                return Common::ErrorCodeValue::Success;
            }
        }
    }
}

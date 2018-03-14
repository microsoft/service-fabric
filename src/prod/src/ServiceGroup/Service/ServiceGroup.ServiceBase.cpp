// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ServiceGroup.Constants.h"
#include "ServiceGroup.ServiceBase.h"

using namespace ServiceGroup;

//
// Constructor/Destructor.
//
CBaseServiceGroup::CBaseServiceGroup(void)
{
}

CBaseServiceGroup::~CBaseServiceGroup(void)
{
}

//
// Refreshes load metric information based on new data.
//
HRESULT CBaseServiceGroup::UpdateLoadMetrics(
    __in FABRIC_PARTITION_ID partitionId, 
    __in ULONG metricCount, 
    __in const FABRIC_LOAD_METRIC* metrics,
    __out FABRIC_LOAD_METRIC** computedMetrics)
{

    ServiceGroupCommonEventSource::GetEvents().Info_0_ServiceGroupCommon(
        partitionIdString_,
        reinterpret_cast<uintptr_t>(this)
        );

    Common::AcquireWriteLock lockPartition(lock_);

    HRESULT hr = S_OK;
    Common::Guid serviceReporting(partitionId);
    std::map<Common::Guid, std::map<std::wstring, ULONG>>::iterator lookupServiceReporting = loadReporting_.find(serviceReporting);
    if (loadReporting_.end() == lookupServiceReporting)
    {
        std::map<std::wstring, ULONG> newMetrics;
        for (ULONG indexMetric = 0; indexMetric < metricCount; indexMetric++)
        {
            auto error = Common::ParameterValidator::IsValid(metrics[indexMetric].Name, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
            if (!error.IsSuccess())
            {
                ServiceGroupCommonEventSource::GetEvents().Error_0_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this)
                    );
    
                return error.ToHResult();
            }
            try
            {
                std::wstring metricName(metrics[indexMetric].Name);
                if (newMetrics.end() != newMetrics.find(metricName))
                {
                    ServiceGroupCommonEventSource::GetEvents().Warning_0_ServiceGroupCommon(
                        partitionIdString_,
                        reinterpret_cast<uintptr_t>(this),
                        metrics[indexMetric].Name
                        );

                    newMetrics[metricName] = metrics[indexMetric].Value;
                }
                else
                {
                    newMetrics.insert(MetricName_MetricValue_Pair(metricName, metrics[indexMetric].Value));
                }

                ServiceGroupCommonEventSource::GetEvents().Info_1_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    metricName.c_str(),
                    metrics[indexMetric].Value,
                    serviceReporting
                    );
            }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupCommonEventSource::GetEvents().Error_1_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this)
                    );

                return hr;
            }
        }
        try
        {
            loadReporting_.insert(Service_Metric_Pair(serviceReporting, newMetrics));
        }
        catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
        catch (...) { hr = E_FAIL; }
        if (FAILED(hr))
        {
            ServiceGroupCommonEventSource::GetEvents().Error_1_ServiceGroupCommon(
                partitionIdString_,
                reinterpret_cast<uintptr_t>(this)
                );
            
            return hr;
        }
    }
    else
    {
        for (ULONG indexMetric = 0; indexMetric < metricCount; indexMetric++)
        {
            auto error = Common::ParameterValidator::IsValid(metrics[indexMetric].Name, Common::ParameterValidator::MinStringSize, Common::ParameterValidator::MaxNameSize);
            if (!error.IsSuccess())
            {
                ServiceGroupCommonEventSource::GetEvents().Error_0_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this)
                    );
    
                return error.ToHResult();
            }
            try
            {
                std::wstring metricName(metrics[indexMetric].Name);
                if (lookupServiceReporting->second.end() != lookupServiceReporting->second.find(metricName))
                {
                    if (lookupServiceReporting->second[metricName] == metrics[indexMetric].Value)
                    {
                        ServiceGroupCommonEventSource::GetEvents().Info_2_ServiceGroupCommon(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this),
                            metricName.c_str(),
                            metrics[indexMetric].Value,
                            serviceReporting
                            );
                    }

                    lookupServiceReporting->second[metricName] = metrics[indexMetric].Value;
                }
                else
                {
                    lookupServiceReporting->second.insert(MetricName_MetricValue_Pair(metricName, metrics[indexMetric].Value));
                }

                ServiceGroupCommonEventSource::GetEvents().Info_1_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this),
                    metricName.c_str(),
                    metrics[indexMetric].Value,
                    serviceReporting
                    );
            }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupCommonEventSource::GetEvents().Error_2_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this)
                    );

                return hr;
            }
        }
    }
    FABRIC_LOAD_METRIC* updatedMetrics = new (std::nothrow) FABRIC_LOAD_METRIC[metricCount];
    if (NULL == updatedMetrics)
    {
       return E_OUTOFMEMORY;
    }
    for (ULONG indexMetric = 0; indexMetric < metricCount; indexMetric++)
    {
        updatedMetrics[indexMetric].Name = metrics[indexMetric].Name;
        updatedMetrics[indexMetric].Value = 0;
        updatedMetrics[indexMetric].Reserved = NULL;
        
        for (std::map<Common::Guid, std::map<std::wstring, ULONG>>::iterator iterate = loadReporting_.begin(); loadReporting_.end() != iterate; iterate++)
        {
            try
            {
                std::wstring metricName(updatedMetrics[indexMetric].Name);
                if (iterate->second.end() != iterate->second.find(metricName))
                {
                    if (updatedMetrics[indexMetric].Value > ~iterate->second[metricName])
                    {
                        ServiceGroupCommonEventSource::GetEvents().Error_3_ServiceGroupCommon(
                            partitionIdString_,
                            reinterpret_cast<uintptr_t>(this)
                            );
                        hr = E_INVALIDARG;
                        break;
                    }
                    updatedMetrics[indexMetric].Value += iterate->second[metricName];
                }
            }
            catch (std::bad_alloc const &) { hr = E_OUTOFMEMORY; }
            catch (...) { hr = E_FAIL; }
            if (FAILED(hr))
            {
                ServiceGroupCommonEventSource::GetEvents().Error_4_ServiceGroupCommon(
                    partitionIdString_,
                    reinterpret_cast<uintptr_t>(this)
                    );
        
                delete[] updatedMetrics;
                return hr;
            }
        }

        ServiceGroupCommonEventSource::GetEvents().Info_3_ServiceGroupCommon(
            partitionIdString_,
            reinterpret_cast<uintptr_t>(this),
            updatedMetrics[indexMetric].Name,
            updatedMetrics[indexMetric].Value
            );
    }
    *computedMetrics = updatedMetrics;
    return hr;
}

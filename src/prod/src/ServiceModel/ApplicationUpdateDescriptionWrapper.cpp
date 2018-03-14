// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationUpdateDescriptionWrapper::ApplicationUpdateDescriptionWrapper()
    : applicationName_()
    , flags_(0)
    , removeApplicationCapacity_(false)
    , minimumNodes_(0)
    , maximumNodes_(0)
    , metrics_()
{
}

ApplicationUpdateDescriptionWrapper::ApplicationUpdateDescriptionWrapper(
    std::wstring const &appName,
    bool removeApplicationCapacity,
    uint mimimumScaleoutCount,
    uint maximumNodes,
    std::vector<Reliability::ApplicationMetricDescription> const & metrics)
    : applicationName_(appName)
    , flags_(0)
    , removeApplicationCapacity_(removeApplicationCapacity)
    , minimumNodes_(mimimumScaleoutCount)
    , maximumNodes_(maximumNodes)
    , metrics_(metrics)
{
}

ApplicationUpdateDescriptionWrapper::ApplicationUpdateDescriptionWrapper(
    ApplicationUpdateDescriptionWrapper const & other)
    : flags_(other.flags_)
    , removeApplicationCapacity_(other.removeApplicationCapacity_)
    , applicationName_(other.applicationName_)
    , minimumNodes_(other.minimumNodes_)
    , maximumNodes_(other.maximumNodes_)
    , metrics_(other.metrics_)
{
}

Common::ErrorCode ApplicationUpdateDescriptionWrapper::FromPublicApi(__in FABRIC_APPLICATION_UPDATE_DESCRIPTION const &appDesc)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(appDesc.ApplicationName, false, applicationName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    flags_ = appDesc.Flags;

    minimumNodes_ = appDesc.MinimumNodes;
    maximumNodes_ = appDesc.MaximumNodes;
    removeApplicationCapacity_ = appDesc.RemoveApplicationCapacity == FALSE ? false : true;

    if (appDesc.Metrics != NULL)
    {
            uint scaleoutMetricCount = appDesc.Metrics->Count;
            for (uint i = 0; i < scaleoutMetricCount; i++)
            {
                Reliability::ApplicationMetricDescription metric;
                ErrorCode error = metric.FromPublicApi(appDesc.Metrics->Capacities[i]);
                if (!error.IsSuccess())
                {
                    return error;
                }
                metrics_.push_back(metric);
            }
        }

    return ErrorCode::Success();
}

bool ApplicationUpdateDescriptionWrapper::TryValidate(__out wstring & validationErrorMessage) const
{
    if (RemoveApplicationCapacity && (UpdateMinimumNodes || UpdateMaximumNodes || UpdateMetrics))
    {
        validationErrorMessage = L"RemoveApplicationCapacity cannot be specified with other parameters.";
        return false;
    }

    if (UpdateMaximumNodes && UpdateMinimumNodes && (MaximumNodes < MinimumNodes))
    {
        validationErrorMessage = wformatString("MinimumNodes ({0}) is greater than MaximumNodes ({1})",
            minimumNodes_,
            maximumNodes_);
        return false;
    }

    for (auto metric = metrics_.begin(); metric != metrics_.end(); metric++)
    {
        wstring tempErrorMessage;
        // If we are updating MaximumNodes, validate with the new value. If not, then validate with 0.
        // Additional validation in CM will happen to check if metrics are valid with correct MaximumNodes
        if (!metric->TryValidate(tempErrorMessage, (UpdateMaximumNodes ? MaximumNodes : 0)))
        {
            validationErrorMessage = wformatString("Error with metric: {0}", tempErrorMessage);
            return false;
        }
    }

    return true;
}


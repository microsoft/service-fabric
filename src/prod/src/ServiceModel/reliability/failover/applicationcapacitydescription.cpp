// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

ApplicationCapacityDescription::ApplicationCapacityDescription()
    : minimumNodes_(0)
    , maximumNodes_(0)
    , metrics_()
{}

ApplicationCapacityDescription::ApplicationCapacityDescription(
    uint minimumNodes,
    uint maximumNodes,
    std::vector<Reliability::ApplicationMetricDescription> metrics)
    : minimumNodes_(minimumNodes)
    , maximumNodes_(maximumNodes)
    , metrics_(metrics)
{
}

ApplicationCapacityDescription::ApplicationCapacityDescription(ApplicationCapacityDescription const& other)
    : minimumNodes_(other.minimumNodes_)
    , maximumNodes_(other.maximumNodes_)
    , metrics_(other.metrics_)
{
}

ApplicationCapacityDescription::ApplicationCapacityDescription(ApplicationCapacityDescription && other)
    : minimumNodes_(other.minimumNodes_)
    , maximumNodes_(other.maximumNodes_)
    , metrics_(move(other.metrics_))
{
}

ApplicationCapacityDescription & ApplicationCapacityDescription::operator = (ApplicationCapacityDescription const& other)
{
    if (this != &other)
    {
        minimumNodes_ = other.minimumNodes_;
        maximumNodes_ = other.maximumNodes_;
        metrics_ = other.metrics_;
    }

    return *this;
}

ApplicationCapacityDescription & ApplicationCapacityDescription::operator = (ApplicationCapacityDescription && other)
{
    if (this != &other)
    {
        minimumNodes_ = other.minimumNodes_;
        maximumNodes_ = other.maximumNodes_;
        metrics_ = move(other.metrics_);
    }

    return *this;
}

bool ApplicationCapacityDescription::operator == (ApplicationCapacityDescription const & other) const
{
    bool equal = minimumNodes_ == other.minimumNodes_ && maximumNodes_ == other.maximumNodes_;
    if (metrics_.size() != other.metrics_.size())
    {
        equal = false;
    }

    for (int i = 0; equal && i < metrics_.size(); i++)
    {
        equal = equal && (metrics_[i] == other.metrics_[i]);
    }

    return equal;
}

bool ApplicationCapacityDescription::operator != (ApplicationCapacityDescription const & other) const
{
    return !(*this == other);
}

Common::ErrorCode ApplicationCapacityDescription::FromPublicApi(__in const FABRIC_APPLICATION_CAPACITY_DESCRIPTION * nativeDescription)
{
    minimumNodes_ = nativeDescription->MinimumNodes;
    maximumNodes_ = nativeDescription->MaximumNodes;
    if (nativeDescription->Metrics != NULL)
    {
        uint scaleoutMetricCount = nativeDescription->Metrics->Count;
        for (uint i = 0; i < scaleoutMetricCount; i++)
        {
            Reliability::ApplicationMetricDescription metric;
            ErrorCode error = metric.FromPublicApi(nativeDescription->Metrics->Capacities[i]);
            if (!error.IsSuccess())
            {
                return error;
            }
            metrics_.push_back(metric);
        }
    }

    return ErrorCode::Success();
}

bool ApplicationCapacityDescription::TryValidate(__out wstring & validationErrorMessage) const
{
    ErrorCode error;
    if (minimumNodes_ > maximumNodes_)
    {
        validationErrorMessage = wformatString("MinimumNodes ({0}) is greater than MaximumNodes ({1})",
            minimumNodes_,
            maximumNodes_);
        return false;
    }

    wstring tempMessage;

    for (auto m : metrics_)
    {
         if (!m.TryValidate(tempMessage, maximumNodes_))
        {
            validationErrorMessage = wformatString("Error with metric: {0}", tempMessage);
            return false;
        }
    }

    return true;
}

void ApplicationCapacityDescription::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} ({2})", minimumNodes_, maximumNodes_, metrics_);
}

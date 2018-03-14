// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

ApplicationMetricDescription::ApplicationMetricDescription()
    : name_(),
    reservationNodeCapacity_(0),
    maximumNodeCapacity_(0),
    totalApplicationCapacity_(0)
{}

ApplicationMetricDescription::ApplicationMetricDescription(
    std::wstring const& name,
    uint reservationNodeCapacity,
    uint maximumNodeCapacity,
    uint totalApplicationCapacity)
    : name_(name),
    reservationNodeCapacity_(reservationNodeCapacity),
    maximumNodeCapacity_(maximumNodeCapacity),
    totalApplicationCapacity_(totalApplicationCapacity)
{}

ApplicationMetricDescription::ApplicationMetricDescription(ApplicationMetricDescription const& other)
    : name_(other.name_),
    reservationNodeCapacity_(other.reservationNodeCapacity_),
    maximumNodeCapacity_(other.maximumNodeCapacity_),
    totalApplicationCapacity_(other.totalApplicationCapacity_)
{}

ApplicationMetricDescription::ApplicationMetricDescription(ApplicationMetricDescription && other)
    : name_(std::move(other.name_)),
    reservationNodeCapacity_(other.reservationNodeCapacity_),
    maximumNodeCapacity_(other.maximumNodeCapacity_),
    totalApplicationCapacity_(other.totalApplicationCapacity_)
{}

ApplicationMetricDescription & ApplicationMetricDescription::operator = (ApplicationMetricDescription const& other)
{
    if (this != &other)
    {
        name_ = other.name_;
        reservationNodeCapacity_ = other.reservationNodeCapacity_;
        maximumNodeCapacity_ = other.maximumNodeCapacity_;
        totalApplicationCapacity_ = other.totalApplicationCapacity_;
    }

    return *this;
}

ApplicationMetricDescription & ApplicationMetricDescription::operator = (ApplicationMetricDescription && other)
{
    if (this != &other)
    {
        name_ = std::move(other.name_);
        reservationNodeCapacity_ = other.reservationNodeCapacity_;
        maximumNodeCapacity_ = other.maximumNodeCapacity_;
        totalApplicationCapacity_ = other.totalApplicationCapacity_;
    }

    return *this;
}

bool ApplicationMetricDescription::operator == (ApplicationMetricDescription const & other) const
{
    return name_ == other.name_
        && reservationNodeCapacity_ == other.reservationNodeCapacity_
        && maximumNodeCapacity_ == other.maximumNodeCapacity_
        && totalApplicationCapacity_ == other.totalApplicationCapacity_;
}

bool ApplicationMetricDescription::operator != (ApplicationMetricDescription const & other) const
{
    return !(*this == other);
}

Common::ErrorCode ApplicationMetricDescription::FromPublicApi(__in FABRIC_APPLICATION_METRIC_DESCRIPTION const & nativeDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring(nativeDescription.Name, false, name_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    reservationNodeCapacity_ = (uint)nativeDescription.NodeReservationCapacity;
    maximumNodeCapacity_ = (uint)nativeDescription.MaximumNodeCapacity;
    totalApplicationCapacity_ = (uint)nativeDescription.TotalApplicationCapacity;

    return ErrorCode::Success();
}

bool ApplicationMetricDescription::TryValidate(__out wstring & validationErrorMessage, uint maximumNodes) const
{
    if (name_.size() == 0)
    {
        validationErrorMessage = L"Metric name must have size greater than zero";
        return false;
    }

    if (reservationNodeCapacity_ > maximumNodeCapacity_)
    {
        validationErrorMessage = wformatString("Metric:{0} Reservation Capacity ({1}) is greater than Maximum Capacity ({2})",
            name_,
            reservationNodeCapacity_,
            maximumNodeCapacity_);
        return false;
    }

    if (maximumNodes * maximumNodeCapacity_ > totalApplicationCapacity_)
    {
        validationErrorMessage = wformatString("Metric:{0} TotalApplicationCapacity ({1}) is smaller than MaximumNodeCapacity * MaximumNodes ({2} * {3}) ",
            name_,
            totalApplicationCapacity_,
            maximumNodeCapacity_,
            maximumNodes);
        return false;
    }

    return true;
}

void ApplicationMetricDescription::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.Write("{0} {1} {2} {3}", name_, reservationNodeCapacity_, maximumNodeCapacity_, totalApplicationCapacity_);
}

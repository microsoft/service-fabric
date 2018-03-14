// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationLoadMetricInformation::ApplicationLoadMetricInformation()
{
}

ApplicationLoadMetricInformation::ApplicationLoadMetricInformation(ApplicationLoadMetricInformation const & other)
    : name_(other.name_)
    , reservationCapacity_(other.reservationCapacity_)
    , applicationCapacity_(other.applicationCapacity_)
    , applicationLoad_(other.applicationLoad_)
{
}

ApplicationLoadMetricInformation::ApplicationLoadMetricInformation(ApplicationLoadMetricInformation && other)
    : name_(move(other.name_))
    , reservationCapacity_(other.reservationCapacity_)
    , applicationCapacity_(other.applicationCapacity_)
    , applicationLoad_(other.applicationLoad_)
{
}

ApplicationLoadMetricInformation const & ApplicationLoadMetricInformation::operator = (ApplicationLoadMetricInformation const & other)
{
    if (this != & other)
    {
        name_ = other.name_;
        applicationCapacity_ = other.applicationCapacity_;
        reservationCapacity_ = other.reservationCapacity_;
        applicationLoad_ = other.applicationLoad_;
    }

    return *this;
}

ApplicationLoadMetricInformation const & ApplicationLoadMetricInformation::operator=(ApplicationLoadMetricInformation && other)
{
    if (this != &other)
    {
        name_ = move(other.name_);
        applicationCapacity_ = other.applicationCapacity_;
        reservationCapacity_ = other.reservationCapacity_;
        applicationLoad_ = other.applicationLoad_;
    }

    return *this;
}

void ApplicationLoadMetricInformation::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_APPLICATION_LOAD_METRIC_INFORMATION & publicValue) const
{
    publicValue.Name = heap.AddString(name_);
    publicValue.ReservationCapacity = reservationCapacity_;
    publicValue.ApplicationCapacity = applicationCapacity_;
    publicValue.ApplicationLoad = applicationLoad_;
}

Common::ErrorCode ApplicationLoadMetricInformation::FromPublicApi(__in FABRIC_APPLICATION_LOAD_METRIC_INFORMATION const & publicValue)
{
    auto hr = StringUtility::LpcwstrToWstring(publicValue.Name, false, name_);
    if (FAILED(hr))
    {
        return ErrorCode::FromHResult(hr);
    }

    reservationCapacity_ = publicValue.ReservationCapacity;
    applicationCapacity_ = publicValue.ApplicationCapacity;
    applicationLoad_ = publicValue.ApplicationLoad;

    return ErrorCode(ErrorCodeValue::Success);
}

void ApplicationLoadMetricInformation::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(ToString());
}

std::wstring ApplicationLoadMetricInformation::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationLoadMetricInformation&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }
    return objectString;
}

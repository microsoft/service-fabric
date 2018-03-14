// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GlobalWString ServiceTypeInstanceIdentifier::Delimiter = make_global<wstring>(L":");

INITIALIZE_SIZE_ESTIMATION(ServiceTypeInstanceIdentifier)

ServiceTypeInstanceIdentifier::ServiceTypeInstanceIdentifier()
    : packageInstanceIdentifier_()
    , serviceTypeName_()
    , serviceTypeId_()
{
}

ServiceTypeInstanceIdentifier::ServiceTypeInstanceIdentifier(
    ServiceTypeIdentifier const & typeIdentifier,
    ServicePackageActivationContext const & activationContext,
	std::wstring const & servicePackageActivationId)
    : packageInstanceIdentifier_(typeIdentifier.ServicePackageId, activationContext, servicePackageActivationId)
    , serviceTypeName_(typeIdentifier.ServiceTypeName)
    , serviceTypeId_(typeIdentifier)
{
}

ServiceTypeInstanceIdentifier::ServiceTypeInstanceIdentifier(
    ServicePackageInstanceIdentifier const & packageInstanceIdentifier,
    wstring const & serviceTypeName)
    : packageInstanceIdentifier_(packageInstanceIdentifier)
    , serviceTypeName_(serviceTypeName)
    , serviceTypeId_(ServiceTypeIdentifier(packageInstanceIdentifier_.ServicePackageId, serviceTypeName_))
{
}

ServiceTypeInstanceIdentifier::ServiceTypeInstanceIdentifier(ServiceTypeInstanceIdentifier const & other)
    : packageInstanceIdentifier_(other.packageInstanceIdentifier_)
    , serviceTypeName_(other.serviceTypeName_)
    , serviceTypeId_(other.serviceTypeId_)
{
}

ServiceTypeInstanceIdentifier::ServiceTypeInstanceIdentifier(ServiceTypeInstanceIdentifier && other)
    : packageInstanceIdentifier_(move(other.packageInstanceIdentifier_))
    , serviceTypeName_(move(other.serviceTypeName_))
    , serviceTypeId_(move(other.serviceTypeId_))
{
}

ServiceTypeInstanceIdentifier const & ServiceTypeInstanceIdentifier::operator = (ServiceTypeInstanceIdentifier const & other)
{
    if (this != &other)
    {
        this->packageInstanceIdentifier_ = other.packageInstanceIdentifier_;
        this->serviceTypeName_ = other.serviceTypeName_;
        this->serviceTypeId_ = other.serviceTypeId_;
    }

    return *this;
}

ServiceTypeInstanceIdentifier const & ServiceTypeInstanceIdentifier::operator = (ServiceTypeInstanceIdentifier && other)
{
    if (this != &other)
    {
        this->packageInstanceIdentifier_ = move(other.packageInstanceIdentifier_);
        this->serviceTypeName_ = move(other.serviceTypeName_);
        this->serviceTypeId_ = move(other.serviceTypeId_);
    }

    return *this;
}

bool ServiceTypeInstanceIdentifier::operator == (ServiceTypeInstanceIdentifier const & other) const
{
    bool equals = true;

    equals = (this->packageInstanceIdentifier_ == other.packageInstanceIdentifier_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->serviceTypeName_, other.serviceTypeName_);
    if (!equals) { return equals; }

    return equals;
}

bool ServiceTypeInstanceIdentifier::operator != (ServiceTypeInstanceIdentifier const & other) const
{
    return !(*this == other);
}

int ServiceTypeInstanceIdentifier::compare(ServiceTypeInstanceIdentifier const & other) const
{
    int comparision = 0;

    comparision = this->packageInstanceIdentifier_.compare(other.packageInstanceIdentifier_);
    if (comparision != 0) { return comparision; }

    comparision = StringUtility::CompareCaseInsensitive(this->serviceTypeName_, other.serviceTypeName_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool ServiceTypeInstanceIdentifier::operator < (ServiceTypeInstanceIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void ServiceTypeInstanceIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServiceTypeInstanceIdentifier::ToString() const
{
    if (packageInstanceIdentifier_.ServicePackageId.IsEmpty())
    {
        return serviceTypeName_;
    }
    else
    {
        return wformatString("{0}{1}{2}", packageInstanceIdentifier_.ToString(), Delimiter, serviceTypeName_);
    }
}

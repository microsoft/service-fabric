// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ServiceCorrelationDescription)

ServiceCorrelationDescription::ServiceCorrelationDescription()
    : serviceName_(), 
    scheme_(FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY)
{
}

ServiceCorrelationDescription::ServiceCorrelationDescription(
    std::wstring const& serviceName, 
    FABRIC_SERVICE_CORRELATION_SCHEME scheme)
    : serviceName_(serviceName), 
    scheme_(scheme)
{
}

ServiceCorrelationDescription::ServiceCorrelationDescription(ServiceCorrelationDescription const& other)
    : serviceName_(other.serviceName_), 
    scheme_(other.scheme_)
{
}

ServiceCorrelationDescription::ServiceCorrelationDescription(ServiceCorrelationDescription && other)
    : serviceName_(std::move(other.serviceName_)), 
    scheme_(other.scheme_)
{
}

ServiceCorrelationDescription & ServiceCorrelationDescription::operator = (ServiceCorrelationDescription const& other)
{
    if (this != &other)
    {
        serviceName_ = other.serviceName_;
        scheme_ = other.scheme_;
    }

    return *this;
}

ServiceCorrelationDescription & ServiceCorrelationDescription::operator = (ServiceCorrelationDescription && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        scheme_ = other.scheme_;
    }

    return *this;
}

bool ServiceCorrelationDescription::operator == (ServiceCorrelationDescription const & other) const
{

    if (!StringUtility::AreEqualCaseInsensitive(this->ServiceName, other.ServiceName))
    {
        return false;
    }

    if (this->Scheme != other.Scheme)
    {
        return false;
    }

    return true;
}

void ServiceCorrelationDescription::WriteToEtw(uint16 contextSequenceId) const
{
    ServiceModel::ServiceModelEventSource::Trace->ServiceCorrelationDescription(
        contextSequenceId,
        serviceName_,
        static_cast<int>(scheme_));
}

void ServiceCorrelationDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1}", serviceName_, static_cast<uint>(scheme_));
}

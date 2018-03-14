// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

ServiceTypeInfo::ServiceTypeInfo()
{
}

ServiceTypeInfo::ServiceTypeInfo(
    ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
    std::wstring const & codePackageName) : 
    versionedServiceTypeId_(versionedServiceTypeId),
    codePackageName_(codePackageName)
{
}

ServiceTypeInfo::ServiceTypeInfo(ServiceTypeInfo const & other) :
versionedServiceTypeId_(other.versionedServiceTypeId_),
codePackageName_(other.codePackageName_)
{
}

ServiceTypeInfo & ServiceTypeInfo::operator=(ServiceTypeInfo const & other)
{
    if (this != &other)
    {
        versionedServiceTypeId_ = other.versionedServiceTypeId_;
        codePackageName_ = other.codePackageName_;
    }

    return *this;
}

ServiceTypeInfo::ServiceTypeInfo(ServiceTypeInfo && other) :
versionedServiceTypeId_(std::move(other.versionedServiceTypeId_)),
codePackageName_(std::move(other.codePackageName_))
{
}

ServiceTypeInfo & ServiceTypeInfo::operator=(ServiceTypeInfo && other)
{
    if (this != &other)
    {
        versionedServiceTypeId_ = std::move(other.versionedServiceTypeId_);
        codePackageName_ = std::move(other.codePackageName_);
    }

    return *this;
}

void ServiceTypeInfo::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w << wformatString("[ServiceTypeId: {0}, CodePackage: {1}]", versionedServiceTypeId_, codePackageName_);
}

void ServiceTypeInfo::WriteToEtw(uint16 contextSequenceId) const
{
    ReliabilityEventSource::Events->ServiceTypeInfo(contextSequenceId, versionedServiceTypeId_, codePackageName_);
}

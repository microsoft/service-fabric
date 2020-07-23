// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

GlobalWString ServiceTypeIdentifier::Delimiter = make_global<wstring>(L":");

Global<ServiceTypeIdentifier> ServiceTypeIdentifier::FailoverManagerServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(), L"FMServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::ClusterManagerServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(), L"ClusterManagerServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::NamingStoreServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(), L"NamingStoreService");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::TombStoneServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(), L"_TSType_");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::FileStoreServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"FileStoreService"), L"FileStoreServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::RepairManagerServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"RM"), L"RepairManagerServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::NamespaceManagerServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"NM"), L"NamespaceManagerServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::FaultAnalysisServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"FAS"), L"FaultAnalysisServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::BackupRestoreServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"BRS"), L"BackupRestoreServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::UpgradeOrchestrationServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"UOS"), L"UpgradeOrchestrationServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::CentralSecretServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"CSS"), L"CentralSecretServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::EventStoreServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"ES"), L"EventStoreServiceType");
Global<ServiceTypeIdentifier> ServiceTypeIdentifier::GatewayResourceManagerServiceTypeId = make_global<ServiceTypeIdentifier>(ServicePackageIdentifier(ApplicationIdentifier(L"__FabricSystem", 4294967295), L"GRM"), L"GatewayResourceManagerType");

INITIALIZE_SIZE_ESTIMATION(ServiceTypeIdentifier)

ServiceTypeIdentifier::ServiceTypeIdentifier()
    : packageIdentifier_(),
    serviceTypeName_()
{
}

ServiceTypeIdentifier::ServiceTypeIdentifier(
    ServicePackageIdentifier const & packageIdentifier, 
    wstring const & serviceTypeName)
    : packageIdentifier_(packageIdentifier),
    serviceTypeName_(serviceTypeName)
{
}

ServiceTypeIdentifier::ServiceTypeIdentifier(ServiceTypeIdentifier const & other)
    : packageIdentifier_(other.packageIdentifier_),
    serviceTypeName_(other.serviceTypeName_)
{
}

ServiceTypeIdentifier::ServiceTypeIdentifier(ServiceTypeIdentifier && other)
    : packageIdentifier_(move(other.packageIdentifier_)),
    serviceTypeName_(move(other.serviceTypeName_))
{
}

ServiceTypeIdentifier const & ServiceTypeIdentifier::operator = (ServiceTypeIdentifier const & other)
{
    if (this != &other)
    {
        this->packageIdentifier_ = other.packageIdentifier_;
        this->serviceTypeName_ = other.serviceTypeName_;
    }

    return *this;
}

ServiceTypeIdentifier const & ServiceTypeIdentifier::operator = (ServiceTypeIdentifier && other)
{
    if (this != &other)
    {
        this->packageIdentifier_ = move(other.packageIdentifier_);
        this->serviceTypeName_ = move(other.serviceTypeName_);
    }

    return *this;
}

bool ServiceTypeIdentifier::operator == (ServiceTypeIdentifier const & other) const
{
    bool equals = true;

    if (!this->packageIdentifier_.ApplicationId.IsEmpty() || !other.packageIdentifier_.ApplicationId.IsEmpty())
    {
        equals = (this->packageIdentifier_ == other.packageIdentifier_);
        if (!equals) { return equals; }
    }

    equals = StringUtility::AreEqualCaseInsensitive(this->serviceTypeName_, other.serviceTypeName_);
    if (!equals) { return equals; }

    return equals;
}

bool ServiceTypeIdentifier::operator != (ServiceTypeIdentifier const & other) const
{
    return !(*this == other);
}

int ServiceTypeIdentifier::compare(ServiceTypeIdentifier const & other) const
{
    int comparision = 0;
    
    if (!this->packageIdentifier_.ApplicationId.IsEmpty() || !other.packageIdentifier_.ApplicationId.IsEmpty())
    {
        comparision = this->packageIdentifier_.compare(other.packageIdentifier_);
        if (comparision != 0) { return comparision; }
    }

    comparision = StringUtility::CompareCaseInsensitive(this->serviceTypeName_, other.serviceTypeName_);
    if (comparision != 0) { return comparision; }

    return comparision;
}

bool ServiceTypeIdentifier::operator < (ServiceTypeIdentifier const & other) const
{
    return (this->compare(other) < 0);
}

void ServiceTypeIdentifier::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ServiceTypeIdentifier::ToString() const 
{
    return ServiceTypeIdentifier::GetQualifiedServiceTypeName(packageIdentifier_, serviceTypeName_);
}

ErrorCode ServiceTypeIdentifier::FromString(
    wstring const & qualifiedServiceTypeName, 
    __out ServiceTypeIdentifier & serviceTypeIdentifier)
{
    auto pos = qualifiedServiceTypeName.find_last_of(*ServiceTypeIdentifier::Delimiter);
    if (pos == wstring::npos)
    {
        serviceTypeIdentifier = ServiceTypeIdentifier(ServicePackageIdentifier(), qualifiedServiceTypeName);
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        wstring servicePackageIdString = qualifiedServiceTypeName.substr(0, pos);
        wstring serviceTypeName = qualifiedServiceTypeName.substr(pos + 1);

        ServicePackageIdentifier servicePackageId;
        auto error = ServicePackageIdentifier::FromString(servicePackageIdString, servicePackageId);
        if (!error.IsSuccess()) { return error; }

        serviceTypeIdentifier = ServiceTypeIdentifier(servicePackageId, serviceTypeName);
        return ErrorCode(ErrorCodeValue::Success);
    }
}

wstring ServiceTypeIdentifier::GetQualifiedServiceTypeName(std::wstring const & applicationId, wstring const & servicePackageName, wstring const & serviceTypeName)
{
    if (applicationId.empty())
    {
        return serviceTypeName;
    }
    else
    {
        return ServicePackageIdentifier::GetServicePackageIdentifierString(applicationId, servicePackageName) + *Delimiter + serviceTypeName;
    }
}

wstring ServiceTypeIdentifier::GetQualifiedServiceTypeName(ServicePackageIdentifier const & packageIdentifier, wstring const & serviceTypeName)
{
    if (packageIdentifier.ApplicationId.IsEmpty())
    {
        return serviceTypeName;
    }
    else
    {
        return packageIdentifier.ToString() + *Delimiter + serviceTypeName;
    }
}

bool ServiceTypeIdentifier::IsSystemServiceType() const
{    
    //
    // "Ad-hoc" system services need to be explicitly checked for here.
    // All others should belong to the system application.
    //
    return (*this == ServiceTypeIdentifier::FailoverManagerServiceTypeId ||
        *this == ServiceTypeIdentifier::ClusterManagerServiceTypeId ||
        *this == ServiceTypeIdentifier::NamingStoreServiceTypeId ||
        this->ApplicationId == ApplicationIdentifier::FabricSystemAppId);
}

bool ServiceTypeIdentifier::IsTombStoneServiceType() const
{
    return *this == ServiceTypeIdentifier::TombStoneServiceTypeId;
}

bool ServiceTypeIdentifier::IsAdhocServiceType() const
{
    return this->ApplicationId.IsAdhoc() && !IsSystemServiceType() && !IsTombStoneServiceType();
}

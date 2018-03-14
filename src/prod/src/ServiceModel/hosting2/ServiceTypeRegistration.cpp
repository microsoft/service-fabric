// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServiceTypeRegistrationSPtr ServiceTypeRegistration::Test_Create(
    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
    ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
    ServiceModel::ServicePackageActivationContext const & activationContext,
	wstring const & servicePackagePublicActivationId,
    std::wstring const & hostId,
    std::wstring const & runtimeId,
    std::wstring const & codePackageName,
    int64 servicePackageInstanceId,
    DWORD pid)
{
    return make_shared<ServiceTypeRegistration>(
        serviceTypeId,
        packageVersionInstance,
        activationContext,
		servicePackagePublicActivationId,
        hostId,
        pid,
        runtimeId,
        codePackageName,
		servicePackageInstanceId);
}

ServiceTypeRegistration::ServiceTypeRegistration(
    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
    ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
    ServiceModel::ServicePackageActivationContext const & activationContext,
	wstring const & servicePackagePublicActivationId,
    std::wstring const & hostId,
    DWORD hostProcessId,
    std::wstring const & runtimeId,
    std::wstring const & codePackageName,
    int64 servicePackageInstanceId)
    : versionedServiceTypeId_(VersionedServiceTypeIdentifier(serviceTypeId, packageVersionInstance)),
    hostId_(hostId),
    hostProcessId_(hostProcessId),
    runtimeId_(runtimeId),
	servicePackageInstanceId_(servicePackageInstanceId),
    codePackageName_(codePackageName),
    activationContext_(activationContext),
	servicePackagePublicActivationId_(servicePackagePublicActivationId)
{
}

ServiceTypeRegistration::~ServiceTypeRegistration()
{
}

VersionedServiceTypeIdentifier const & ServiceTypeRegistration::get_VersionedServiceTypeId() const
{
    return versionedServiceTypeId_;
}

ServiceTypeIdentifier const & ServiceTypeRegistration::get_ServiceTypeId() const
{
    return versionedServiceTypeId_.Id;
}

ServicePackageVersionInstance const & ServiceTypeRegistration::get_ServicePackageVersionInstance() const
{
    return versionedServiceTypeId_.servicePackageVersionInstance;
}

wstring const & ServiceTypeRegistration::get_RuntimeId() const 
{ 
    return runtimeId_;
}

wstring const & ServiceTypeRegistration::get_HostId() const 
{ 
    return hostId_;
}

DWORD ServiceTypeRegistration::get_HostProcessId() const 
{ 
    return hostProcessId_;
}

int64 const & ServiceTypeRegistration::get_ServicePackageInstanceId() const
{
    return servicePackageInstanceId_;
}

wstring const & ServiceTypeRegistration::get_CodePackageName() const
{
    return codePackageName_;
}

wstring const & ServiceTypeRegistration::get_ServicePackagePublicActivationId() const
{
	return servicePackagePublicActivationId_;
}

void ServiceTypeRegistration::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("ServiceTypeRegistration {");
    w.Write("VersionedServiceTypeId={0}", VersionedServiceTypeId);
    w.Write("HostId={0}", HostId);
    w.Write("HostProcessId={0}", HostProcessId);
    w.Write("RuntimeId={0}", RuntimeId);
    w.Write("ServicePackageInstanceId={0}", ServicePackageInstanceId);
	w.Write("ServicePackagePublicActivationId={0}", ServicePackagePublicActivationId);
    w.Write("CodePackageName={0}", CodePackageName);
    w.Write("ActivationContext={0}", ActivationContext);
    w.Write("}");
}

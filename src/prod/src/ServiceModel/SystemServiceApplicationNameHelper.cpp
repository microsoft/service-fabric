// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

#define CHECK_INTERNAL_SINGLETON( ServiceName ) \
    StringUtility::Compare(internalServiceName, *Internal##ServiceName##Name) == 0

#define CHECK_INTERNAL_MULTI_INSTANCE( ServiceName ) \
    StringUtility::StartsWith(internalServiceName, *Internal##ServiceName##Name)

#define CHECK_PUBLIC_SINGLETON( ServiceName ) \
    StringUtility::Compare(publicServiceName, *Public##ServiceName##Name) == 0

#define CHECK_PUBLIC_MULTI_INSTANCE( ServiceName ) \
    StringUtility::StartsWith(publicServiceName, *Public##ServiceName##Name)

#define TRY_GET_PUBLIC_SINGLETON_NAME( ServiceName ) \
    if (CHECK_INTERNAL_SINGLETON( ServiceName )) \
    { \
        return Public##ServiceName##Name; \
    } \

#define TRY_GET_PUBLIC_MULTI_INSTANCE_NAME( ServiceName ) \
    if (CHECK_INTERNAL_MULTI_INSTANCE( ServiceName )) \
    { \
        return wformatString("{0}/{1}", *SystemServiceApplicationName, internalServiceName); \
    } \

#define TRY_GET_INTERNAL_SINGLETON_NAME( ServiceName ) \
    if (CHECK_PUBLIC_SINGLETON( ServiceName )) \
    { \
        return Internal##ServiceName##Name; \
    } \

// Remove the "fabric:/System/" prefix.
// e.g. "fabric:/System/InfrastructureService[/InstanceName]" ==> "InfrastructureService[/InstanceName]"
//
#define TRY_GET_INTERNAL_MULTI_INSTANCE_NAME( ServiceName ) \
    if (CHECK_PUBLIC_MULTI_INSTANCE( ServiceName )) \
    { \
        return publicServiceName.substr(SystemServiceApplicationName->length() + 1); \
    } \

#define DEFINE_INTERNAL_NAME( ServiceName, Value ) \
    GlobalWString SystemServiceApplicationNameHelper::Internal##ServiceName##Name = make_global<std::wstring>(COMMON_WSTRINGIFY( Value ));

#define DEFINE_INTERNAL_NAME_PREFIX( ServiceName, Value ) \
    GlobalWString SystemServiceApplicationNameHelper::Internal##ServiceName##Prefix = make_global<std::wstring>(wformatString("{0}/", COMMON_WSTRINGIFY( Value )));

#define DEFINE_TYPE_NAME( ServiceName ) \
    GlobalWString SystemServiceApplicationNameHelper::ServiceName##Type = make_global<std::wstring>(wformatString("{0}Type", COMMON_WSTRINGIFY( ServiceName )));

#define DEFINE_PUBLIC_NAME( ServiceName, Value ) \
    GlobalWString SystemServiceApplicationNameHelper::Public##ServiceName##Name = make_global<std::wstring>(wformatString("{0}/{1}", *SystemServiceApplicationName, COMMON_WSTRINGIFY( Value )));

#define DEFINE_NAMES( ServiceName ) \
    DEFINE_INTERNAL_NAME( ServiceName, ServiceName ) \
    DEFINE_PUBLIC_NAME( ServiceName, ServiceName )

#define DEFINE_PACKAGE_NAME( ServiceName, Value ) \
    GlobalWString SystemServiceApplicationNameHelper::ServiceName##PackageName = make_global<std::wstring>(COMMON_WSTRINGIFY( Value ));

#define DEFINE_SYSTEM_SERVICE_NAMES( ServiceName, PackageName ) \
    DEFINE_NAMES( ServiceName ) \
    DEFINE_PACKAGE_NAME( ServiceName, PackageName ) \
    DEFINE_INTERNAL_NAME_PREFIX( ServiceName, ServiceName ) \
    DEFINE_TYPE_NAME( ServiceName ) \

GlobalWString SystemServiceApplicationNameHelper::SystemServiceApplicationAuthority = make_global<wstring>(L"System");
GlobalWString SystemServiceApplicationNameHelper::SystemServiceApplicationName = make_global<wstring>(wformatString("fabric:/{0}", *SystemServiceApplicationAuthority));

//
// Some system services do not follow the same naming convention
// as other system services, but leave them as is to preserve
// backwards compatibility.
//

DEFINE_INTERNAL_NAME( ClusterManagerService, ClusterManagerServiceName )
DEFINE_PUBLIC_NAME( ClusterManagerService, ClusterManagerService )

DEFINE_INTERNAL_NAME( FMService, FMService )
DEFINE_PUBLIC_NAME( FMService, FailoverManagerService )

DEFINE_NAMES( ImageStoreService )
DEFINE_PACKAGE_NAME( FileStoreService, FileStoreService )

DEFINE_NAMES( NamingService )
DEFINE_NAMES( RepairManagerService )

DEFINE_SYSTEM_SERVICE_NAMES( FaultAnalysisService, FAS)
DEFINE_SYSTEM_SERVICE_NAMES( InfrastructureService, IS)
DEFINE_SYSTEM_SERVICE_NAMES( NamespaceManagerService, NM)
DEFINE_SYSTEM_SERVICE_NAMES( DSTSTokenValidationService, TVS)
DEFINE_SYSTEM_SERVICE_NAMES( TokenValidationService, TVS)
DEFINE_SYSTEM_SERVICE_NAMES( UpgradeOrchestrationService, UOS)
DEFINE_SYSTEM_SERVICE_NAMES( UpgradeService, US)
DEFINE_SYSTEM_SERVICE_NAMES( DnsService, DnsService)
DEFINE_SYSTEM_SERVICE_NAMES( BackupRestoreService, BRS)
DEFINE_SYSTEM_SERVICE_NAMES( ResourceMonitorService, RMS)
DEFINE_SYSTEM_SERVICE_NAMES( CentralSecretService, CSS)
DEFINE_SYSTEM_SERVICE_NAMES( LocalSecretService, LSS)
DEFINE_SYSTEM_SERVICE_NAMES( EventStoreService, ES)
DEFINE_SYSTEM_SERVICE_NAMES( GatewayResourceManager, GRM)

bool SystemServiceApplicationNameHelper::TryGetSystemApplicationName(ServiceTypeIdentifier const & serviceTypeId, __out wstring & applicationName)
{
    if (serviceTypeId.IsSystemServiceType())
    {
        applicationName = *SystemServiceApplicationName;
        return true;
    }

    return false;
}

wstring SystemServiceApplicationNameHelper::GetPublicServiceName(wstring const & internalServiceName)
{
    TRY_GET_PUBLIC_SINGLETON_NAME( ClusterManagerService );
    TRY_GET_PUBLIC_SINGLETON_NAME( FaultAnalysisService );
    TRY_GET_PUBLIC_SINGLETON_NAME( FMService );
    TRY_GET_PUBLIC_SINGLETON_NAME( ImageStoreService );
    TRY_GET_PUBLIC_SINGLETON_NAME( NamingService );
    TRY_GET_PUBLIC_SINGLETON_NAME( RepairManagerService );
    TRY_GET_PUBLIC_SINGLETON_NAME( UpgradeOrchestrationService );
    TRY_GET_PUBLIC_SINGLETON_NAME( UpgradeService );
    TRY_GET_PUBLIC_SINGLETON_NAME( DnsService );
    TRY_GET_PUBLIC_SINGLETON_NAME( BackupRestoreService );
    TRY_GET_PUBLIC_SINGLETON_NAME( ResourceMonitorService );
    TRY_GET_PUBLIC_SINGLETON_NAME( CentralSecretService );
    TRY_GET_PUBLIC_SINGLETON_NAME( LocalSecretService );
    TRY_GET_PUBLIC_SINGLETON_NAME( EventStoreService );

    TRY_GET_PUBLIC_MULTI_INSTANCE_NAME( InfrastructureService );
    TRY_GET_PUBLIC_MULTI_INSTANCE_NAME( DSTSTokenValidationService );
    TRY_GET_PUBLIC_MULTI_INSTANCE_NAME( TokenValidationService );
    TRY_GET_PUBLIC_MULTI_INSTANCE_NAME( GatewayResourceManager );
    return internalServiceName;
}

wstring SystemServiceApplicationNameHelper::GetInternalServiceName(wstring const & publicServiceName)
{
    TRY_GET_INTERNAL_SINGLETON_NAME( ClusterManagerService );
    TRY_GET_INTERNAL_SINGLETON_NAME( FaultAnalysisService );
    TRY_GET_INTERNAL_SINGLETON_NAME( FMService );
    TRY_GET_INTERNAL_SINGLETON_NAME( ImageStoreService );
    TRY_GET_INTERNAL_SINGLETON_NAME( NamingService );
    TRY_GET_INTERNAL_SINGLETON_NAME( RepairManagerService );
    TRY_GET_INTERNAL_SINGLETON_NAME( UpgradeOrchestrationService );
    TRY_GET_INTERNAL_SINGLETON_NAME( UpgradeService );
    TRY_GET_INTERNAL_SINGLETON_NAME( DnsService );
    TRY_GET_INTERNAL_SINGLETON_NAME( BackupRestoreService );
    TRY_GET_INTERNAL_SINGLETON_NAME( ResourceMonitorService );
    TRY_GET_INTERNAL_SINGLETON_NAME( CentralSecretService );
    TRY_GET_INTERNAL_SINGLETON_NAME( LocalSecretService );
    TRY_GET_INTERNAL_SINGLETON_NAME( EventStoreService );

    TRY_GET_INTERNAL_MULTI_INSTANCE_NAME( InfrastructureService );
    TRY_GET_INTERNAL_MULTI_INSTANCE_NAME( DSTSTokenValidationService );
    TRY_GET_INTERNAL_MULTI_INSTANCE_NAME( TokenValidationService );
    TRY_GET_INTERNAL_MULTI_INSTANCE_NAME( GatewayResourceManager );
    return publicServiceName;
}

bool SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(NamingUri const& name)
{
    return IsSystemServiceApplicationName(name.ToString());
}

bool SystemServiceApplicationNameHelper::IsSystemServiceApplicationName(std::wstring const& name)
{
    return (StringUtility::Compare(name, SystemServiceApplicationNameHelper::SystemServiceApplicationName) == 0);
}

bool SystemServiceApplicationNameHelper::IsInternalServiceName(std::wstring const& internalServiceName)
{
    return (
        CHECK_INTERNAL_SINGLETON( ClusterManagerService ) ||
        CHECK_INTERNAL_SINGLETON( FaultAnalysisService ) ||
        CHECK_INTERNAL_SINGLETON( FMService ) ||
        CHECK_INTERNAL_SINGLETON( ImageStoreService ) ||
        CHECK_INTERNAL_SINGLETON( NamingService ) ||
        CHECK_INTERNAL_SINGLETON( RepairManagerService ) ||
        CHECK_INTERNAL_SINGLETON( UpgradeOrchestrationService ) ||
        CHECK_INTERNAL_SINGLETON( UpgradeService ) ||
        CHECK_INTERNAL_SINGLETON( DnsService ) ||
        CHECK_INTERNAL_SINGLETON( BackupRestoreService ) ||
        CHECK_INTERNAL_SINGLETON( ResourceMonitorService ) ||
        CHECK_INTERNAL_SINGLETON( CentralSecretService ) ||
        CHECK_INTERNAL_SINGLETON( LocalSecretService ) ||
        CHECK_INTERNAL_SINGLETON( EventStoreService ) ||

        CHECK_INTERNAL_MULTI_INSTANCE( InfrastructureService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE( DSTSTokenValidationService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE(TokenValidationService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE( GatewayResourceManager ));
}

bool SystemServiceApplicationNameHelper::IsPublicServiceName(std::wstring const& publicServiceName)
{
    return (
        CHECK_PUBLIC_SINGLETON( ClusterManagerService ) ||
        CHECK_PUBLIC_SINGLETON( FaultAnalysisService ) ||
        CHECK_PUBLIC_SINGLETON( FMService ) ||
        CHECK_PUBLIC_SINGLETON( ImageStoreService ) ||
        CHECK_PUBLIC_SINGLETON( NamingService ) ||
        CHECK_PUBLIC_SINGLETON( RepairManagerService ) ||
        CHECK_PUBLIC_SINGLETON( UpgradeOrchestrationService ) ||
        CHECK_PUBLIC_SINGLETON( UpgradeService ) ||
        CHECK_PUBLIC_SINGLETON( DnsService ) ||
        CHECK_PUBLIC_SINGLETON( BackupRestoreService ) ||
        CHECK_PUBLIC_SINGLETON( ResourceMonitorService ) ||
        CHECK_PUBLIC_SINGLETON( CentralSecretService ) ||
        CHECK_PUBLIC_SINGLETON( LocalSecretService ) ||
        CHECK_PUBLIC_SINGLETON( EventStoreService ) ||

        CHECK_PUBLIC_MULTI_INSTANCE( InfrastructureService ) ||
        CHECK_PUBLIC_MULTI_INSTANCE( DSTSTokenValidationService ) ||
        CHECK_PUBLIC_MULTI_INSTANCE( TokenValidationService ) ||
        CHECK_PUBLIC_MULTI_INSTANCE( GatewayResourceManager ));
}

bool SystemServiceApplicationNameHelper::IsSystemServiceName(std::wstring const& serviceName)
{
    return (IsInternalServiceName(serviceName) || IsPublicServiceName(serviceName));
}

bool SystemServiceApplicationNameHelper::IsSystemServiceDeletable(std::wstring const& internalServiceName)
{
    return (
        CHECK_INTERNAL_SINGLETON( UpgradeService ) ||
        CHECK_INTERNAL_SINGLETON( DnsService ) ||
        CHECK_INTERNAL_SINGLETON( ResourceMonitorService ) ||
        CHECK_INTERNAL_SINGLETON( EventStoreService ) ||

        CHECK_INTERNAL_MULTI_INSTANCE( InfrastructureService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE( DSTSTokenValidationService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE( TokenValidationService ) ||
        CHECK_INTERNAL_MULTI_INSTANCE( GatewayResourceManager ));
}

bool SystemServiceApplicationNameHelper::IsNamespaceManagerName(std::wstring const& publicServiceName)
{
    return StringUtility::StartsWith(publicServiceName, *SystemServiceApplicationNameHelper::PublicNamespaceManagerServiceName);
}
 
wstring SystemServiceApplicationNameHelper::CreatePublicNamespaceManagerServiceName(std::wstring const& suffix)
{
    return wformatString("{0}/{1}", SystemServiceApplicationName, CreateInternalNamespaceManagerServiceName(suffix));
}

wstring SystemServiceApplicationNameHelper::CreateInternalNamespaceManagerServiceName(std::wstring const& suffix)
{
    return wformatString("{0}/{1}", InternalNamespaceManagerServiceName, suffix);
}

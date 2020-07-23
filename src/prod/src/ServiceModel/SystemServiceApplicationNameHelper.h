// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

#define DECLARE_NAMES( ServiceName ) \
    static Common::GlobalWString Internal##ServiceName##Name; \
    static Common::GlobalWString Public##ServiceName##Name; \

#define DECLARE_PACKAGE_NAME( ServiceName ) \
    static Common::GlobalWString ServiceName##PackageName; \

#define DECLARE_SYSTEM_SERVICE_NAMES( ServiceName ) \
    DECLARE_NAMES( ServiceName ) \
    DECLARE_PACKAGE_NAME( ServiceName ) \
    static Common::GlobalWString Internal##ServiceName##Prefix; \
    static Common::GlobalWString ServiceName##Type;

namespace ServiceModel
{
    class SystemServiceApplicationNameHelper
    {
    public:

        static Common::GlobalWString SystemServiceApplicationName;
        static Common::GlobalWString SystemServiceApplicationAuthority;

        DECLARE_NAMES( ClusterManagerService )
        DECLARE_NAMES( FMService )

        DECLARE_NAMES( ImageStoreService )
        DECLARE_PACKAGE_NAME( FileStoreService )

        DECLARE_NAMES( NamingService )
        DECLARE_NAMES( RepairManagerService )

        // TokenValidationService was supposed to be a service supporting
        // pluggable validation providers. However, its service name was
        // "DSTSTokenValidationService", where DSTS is a provider.
        //
        // DSTSTokenValidationService exists to provide backwards compatiblity.

        DECLARE_SYSTEM_SERVICE_NAMES( FaultAnalysisService )
        DECLARE_SYSTEM_SERVICE_NAMES( InfrastructureService )
        DECLARE_SYSTEM_SERVICE_NAMES( NamespaceManagerService )
        DECLARE_SYSTEM_SERVICE_NAMES( DSTSTokenValidationService )
        DECLARE_SYSTEM_SERVICE_NAMES( TokenValidationService )
        DECLARE_SYSTEM_SERVICE_NAMES( UpgradeOrchestrationService )
        DECLARE_SYSTEM_SERVICE_NAMES( UpgradeService )
        DECLARE_SYSTEM_SERVICE_NAMES( DnsService )
        DECLARE_SYSTEM_SERVICE_NAMES( BackupRestoreService )
        DECLARE_SYSTEM_SERVICE_NAMES( ResourceMonitorService )
        DECLARE_SYSTEM_SERVICE_NAMES( CentralSecretService )
        DECLARE_SYSTEM_SERVICE_NAMES( LocalSecretService )
        DECLARE_SYSTEM_SERVICE_NAMES( EventStoreService )
        DECLARE_SYSTEM_SERVICE_NAMES( GatewayResourceManager )

    public:
        static bool TryGetSystemApplicationName(ServiceTypeIdentifier const & serviceTypeId, __out std::wstring & applicationName);
        static std::wstring GetPublicServiceName(std::wstring const & internalServiceName);
        static std::wstring GetInternalServiceName(std::wstring const & publicServiceName);

        static bool IsSystemServiceApplicationName(Common::NamingUri const&);
        static bool IsSystemServiceApplicationName(std::wstring const&);
        static bool IsInternalServiceName(std::wstring const&);
        static bool IsPublicServiceName(std::wstring const&);
        static bool IsSystemServiceName(std::wstring const&);
        static bool IsSystemServiceDeletable(std::wstring const&);

        static bool IsNamespaceManagerName(std::wstring const&);
        static std::wstring CreatePublicNamespaceManagerServiceName(std::wstring const&);
        static std::wstring CreateInternalNamespaceManagerServiceName(std::wstring const&);
    };
}

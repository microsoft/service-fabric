// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    // -------------------------------
    // Tracing
    // -------------------------------

    // Trace Sources
    Common::StringLiteral const Constants::CommonSource("Naming.Common");
    Common::StringLiteral const Constants::NamingClientSource("Operation");
    Common::StringLiteral const Constants::NamingServiceSource("Naming.Service");
    Common::StringLiteral const Constants::TestSource("Naming.Test");
    Common::StringLiteral const Constants::FabricClientSource("FabricClient");

    // Tracing
    Common::GlobalWString Constants::NullString = Common::make_global<std::wstring>(L"-");
    Common::GlobalWString Constants::TracingTokenDelimiter = Common::make_global<std::wstring>(L"+");

    // -------------------------------
    // FabricClient General
    // -------------------------------
    LONGLONG Constants::CurrentNotificationHandlerId(0L);

    // -------------------------------
    // Reliability
    // -------------------------------
    
    Common::GlobalWString Constants::NamingServicePrimaryCountName = Common::make_global<std::wstring>(L"__NamingServicePrimaryCount__");
    Common::GlobalWString Constants::NamingServiceReplicaCountName = Common::make_global<std::wstring>(L"__NamingServiceReplicaCount__");

    // -------------------------------
    // Naming Service General
    // -------------------------------

    int const Constants::ScaleoutCount(0);
    uint const Constants::DefaultMoveCost(::FABRIC_MOVE_COST_LOW);
        
    // -------------------------------
    // Store Service
    // -------------------------------
    
    Common::GlobalWString Constants::NamingStoreFilename = Common::make_global<std::wstring>(L"NS.edb");
    Common::GlobalWString Constants::NamingStoreDirectory = Common::make_global<std::wstring>(L"NS");
    Common::GlobalWString Constants::NamingStoreSharedLogFilename = Common::make_global<std::wstring>(L"nsshared.log");
    Common::GlobalWString Constants::HierarchyNameEntryType = Common::make_global<std::wstring>(L"HierarchyName");
    Common::GlobalWString Constants::NonHierarchyNameEntryType = Common::make_global<std::wstring>(L"Name");
    Common::GlobalWString Constants::NamedPropertyType = Common::make_global<std::wstring>(L"Property");
    Common::GlobalWString Constants::UserServiceDataType = Common::make_global<std::wstring>(L"UserServiceData");
    Common::GlobalWString Constants::UserServiceRecoveryDataType = Common::make_global<std::wstring>(L"UserServiceRecoveryData");
    Common::GlobalWString Constants::UserServiceUpdateDataType = Common::make_global<std::wstring>(L"UserServiceUpdateData");

    // -------------------------------
    // Service Group
    // -------------------------------
    Common::GlobalWString Constants::MemberServiceNameDelimiter = Common::make_global<std::wstring>(L"#");
    Common::GlobalWString Constants::ServiceAddressDelimiter = Common::make_global<std::wstring>(L"%");
    Common::GlobalWString Constants::ServiceAddressDoubleDelimiter = Common::make_global<std::wstring>(L"%%");
    Common::GlobalWString Constants::ServiceAddressEscapedDelimiter = Common::make_global<std::wstring>(L"(%)");

    // -------------------------------
    // File Transfer Gateway
    // -------------------------------
    Common::GlobalWString Constants::FileTransferGatewayDirectory = Common::make_global<std::wstring>(L"FileTransfer");
    double const Constants::ChunkFileLockAcquireTimeoutInSeconds = 5.0;
    Common::GlobalWString Constants::FileStoreServiceSingletonPartitionGuid = Common::make_global<std::wstring>(L"00000000-0000-0000-0000-000000003000");
}

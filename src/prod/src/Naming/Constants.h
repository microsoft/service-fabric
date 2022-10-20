// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class Constants
    {
    public:
        // -------------------------------
        // Tracing
        // -------------------------------

        // Trace Sources
        static Common::StringLiteral const CommonSource;
        static Common::StringLiteral const FabricClientSource;
        static Common::StringLiteral const NamingClientSource;
        static Common::StringLiteral const NamingServiceSource;
        static Common::StringLiteral const TestSource;
        
        // Tracing
        static Common::GlobalWString NullString;
        static Common::GlobalWString TracingTokenDelimiter;

        // -------------------------------
        // FabricClient General
        // -------------------------------
        static LONGLONG CurrentNotificationHandlerId;

        // -------------------------------
        // Reliability
        // -------------------------------
        
        static Common::GlobalWString NamingServicePrimaryCountName;
        static Common::GlobalWString NamingServiceReplicaCountName;

        // -------------------------------
        // Naming Service General
        // -------------------------------

        static int const ScaleoutCount;
        static uint const DefaultMoveCost;

        // -------------------------------
        // Store Service
        // -------------------------------
        
        static Common::GlobalWString NamingStoreFilename;
        static Common::GlobalWString NamingStoreDirectory;
        static Common::GlobalWString NamingStoreSharedLogFilename;
        static Common::GlobalWString HierarchyNameEntryType;
        static Common::GlobalWString NonHierarchyNameEntryType;
        static Common::GlobalWString NamedPropertyType;
        static Common::GlobalWString UserServiceDataType;
        static Common::GlobalWString UserServiceRecoveryDataType;
        static Common::GlobalWString UserServiceUpdateDataType;

        // -------------------------------
        // Service Group
        // -------------------------------
        static Common::GlobalWString MemberServiceNameDelimiter;
        static Common::GlobalWString ServiceAddressDelimiter;
        static Common::GlobalWString ServiceAddressDoubleDelimiter;
        static Common::GlobalWString ServiceAddressEscapedDelimiter;

        // -------------------------------
        // File Transfer Gateway
        // -------------------------------
        static Common::GlobalWString FileTransferGatewayDirectory;
        static double const ChunkFileLockAcquireTimeoutInSeconds;
        static Common::GlobalWString FileStoreServiceSingletonPartitionGuid;
    };

}

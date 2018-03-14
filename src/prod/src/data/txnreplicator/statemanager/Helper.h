// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        class Helper
        {
        public:
            enum Component
            {
                StateManager = 0,
                CheckpointManager = 1,
                CheckpointFile = 2,
                MetadataManager =3,
                NamedOperationDataStream = 4
            };

            // Helper function used to cast stateProviderId from LONG64 to KString::SPtr
            static KString::SPtr StateProviderIDToKStringSPtr(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Component component,
                __in KAllocator& allocator);

            static NTSTATUS CreateFolder(__in KStringView const & folderPath);
            static NTSTATUS CreateFolder(
                __in KStringView const & folderPath, 
                __in Common::Guid partitionId, 
                __in FABRIC_REPLICA_ID replicaId) noexcept;

            static NTSTATUS DeleteFolder(__in KStringView const & folderPath);

            static void ThrowIfNecessary(
                __in NTSTATUS status,
                __in Common::Guid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in std::wstring message,
                __in Component component);

            static void ThrowIfNecessary(
                __in Common::ErrorCode errorCode,
                __in Common::Guid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in std::wstring message,
                __in Component component);

            static void TraceErrors(
                __in NTSTATUS status,
                __in Common::Guid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in Component component,
                __in std::wstring message);

            static void TraceErrors(
                __in Common::ErrorCode errorCode,
                __in Common::Guid partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in Component component,
                __in std::wstring message);

            static std::wstring StringFormat(
                __in Common::StringLiteral format);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0,
                __in Common::VariableArgument const & arg1);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0,
                __in Common::VariableArgument const & arg1,
                __in Common::VariableArgument const & arg2);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0,
                __in Common::VariableArgument const & arg1,
                __in Common::VariableArgument const & arg2,
                __in Common::VariableArgument const & arg3);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0,
                __in Common::VariableArgument const & arg1,
                __in Common::VariableArgument const & arg2,
                __in Common::VariableArgument const & arg3,
                __in Common::VariableArgument const & arg4);

            static std::wstring StringFormat(
                __in Common::StringLiteral format,
                __in Common::VariableArgument const & arg0,
                __in Common::VariableArgument const & arg1,
                __in Common::VariableArgument const & arg2,
                __in Common::VariableArgument const & arg3,
                __in Common::VariableArgument const & arg4,
                __in Common::VariableArgument const & arg5);
        };
    }
}

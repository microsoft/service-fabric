// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data;
using namespace Data::Utilities;
using namespace Data::StateManager;

KString::SPtr Helper::StateProviderIDToKStringSPtr(
    __in PartitionedReplicaId const & traceId,
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in Component component,
    __in KAllocator & allocator)
{
    KLocalString<20> stateProviderIdString;
    BOOLEAN boolStatus = stateProviderIdString.FromLONGLONG(stateProviderId);
    ASSERT_IFNOT(
        boolStatus == TRUE,
        "{0}: StateProviderIDToKStringSPtr: Build StateProviderIdString failed. Component: {1}.",
        traceId.TraceId,
        static_cast<int>(component));

    KString::SPtr result;
    NTSTATUS status = KString::Create(result, allocator, stateProviderIdString);
    ThrowIfNecessary(
        status,
        traceId.TracePartitionId,
        traceId.ReplicaId,
        L"StateProviderIDToKStringSPtr: Create StateProviderId String failed.",
        component);

    return result;
}

/*
* #8734667: Use when CreateDirectoryAsync is introduced.
*/
NTSTATUS Helper::CreateFolder(__in KStringView const & folderPath)
{
    if (Common::Directory::Exists(static_cast<LPCWSTR>(folderPath)))
    {
        return STATUS_SUCCESS;
    }

    Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(folderPath));
    if (errorCode.IsSuccess() == false)
    {
        return Utilities::StatusConverter::Convert(errorCode.ToHResult());
    }

    return STATUS_SUCCESS;
}

/*
* #8734667: Use when CreateDirectoryAsync is introduced.
*/
NTSTATUS Helper::CreateFolder(
    __in KStringView const & folderPath,
    __in Common::Guid partitionId,
    __in FABRIC_REPLICA_ID replicaId) noexcept
{
    if (Common::Directory::Exists(static_cast<LPCWSTR>(folderPath)))
    {
        return STATUS_SUCCESS;
    }

    Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(folderPath));
    if (errorCode.IsSuccess() == false)
    {
        wstring message = Common::wformatString(
            "Folder could not be created. Folder Path: {0} ErrorCode: {1}",
            wstring(static_cast<LPCWSTR>(folderPath)),
            errorCode);

        StateManagerEventSource::Events->Warning(
            partitionId,
            replicaId,
            message);

        return Utilities::StatusConverter::Convert(errorCode.ToHResult());
    }

    return STATUS_SUCCESS;
}

NTSTATUS Helper::DeleteFolder(__in KStringView const & folderPath)
{
    if (!Common::Directory::Exists(static_cast<LPCWSTR>(folderPath)))
    {
        return STATUS_SUCCESS;
    }

    Common::ErrorCode errorCode = Common::Directory::Delete(static_cast<LPCWSTR>(folderPath), true);
    if (!errorCode.IsSuccess())
    {
        return Utilities::StatusConverter::Convert(errorCode.ToHResult());
    }

    return STATUS_SUCCESS;

    /*
    Following code gets stuck if synchronizer is used.
    If synchronizer is not used, we cannot guarantee that the folder was deleted.

    NTSTATUS status = STATUS_UNSUCCESSFUL;
    KSynchronizer synchronizer;

    KWString folderName(GetThisAllocator(), folderPath.ToUNICODE_STRING());
    status = KVolumeNamespace::DeleteFileOrDirectory(folderName, GetThisAllocator(), synchronizer);
    THROW_ON_FAILURE(status)

    status = synchronizer.WaitForCompletion();
    THROW_ON_FAILURE(status)
    */
}

void Helper::ThrowIfNecessary(
    __in NTSTATUS status,
    __in Common::Guid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in std::wstring message,
    __in Component component)
{
    if (NT_SUCCESS(status))
    {
        return;
    }

    switch (component) 
    {
        case CheckpointFile:
        {
            StateManagerEventSource::Events->CheckpointFileError(
                partitionId,
                replicaId,
                message,
                status);
            break;
        }
        case CheckpointManager:
        {
            StateManagerEventSource::Events->CheckpointManagerError(
                partitionId,
                replicaId,
                message,
                status);
            break;
        }
        case MetadataManager:
        {
            StateManagerEventSource::Events->MetadataManager_Error(
                partitionId,
                replicaId,
                message,
                status);
            break;
        }
        case StateManager:
        {
            StateManagerEventSource::Events->Error(
                partitionId,
                replicaId,
                message,
                status);
            break;
        }
        case NamedOperationDataStream:
        {
            StateManagerEventSource::Events->NamedOperationDataStreamError(
                partitionId,
                replicaId,
                message,
                status);
            break;
        }
        default:
        {
            ASSERT_IF(true, "PartitionId: {0}: ReplicaId: {1}: ThrowIfNecessary: should not reach the default case.", partitionId, replicaId);
        }
    }

    throw ktl::Exception(status);
}

void Helper::ThrowIfNecessary(
    __in Common::ErrorCode errorCode,
    __in Common::Guid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in std::wstring message,
    __in Component component)
{
    if (errorCode.IsSuccess())
    {
        return;
    }

    switch (component) 
    {
        case CheckpointFile:
        {
            StateManagerEventSource::Events->CheckpointFileError(
                partitionId,
                replicaId,
                message,
                errorCode.ToHResult());
            break;
        }
        case CheckpointManager:
        {
            StateManagerEventSource::Events->CheckpointManagerError(
                partitionId,
                replicaId,
                message,
                errorCode.ToHResult());
            break;
        }
        case MetadataManager:
        {
            StateManagerEventSource::Events->MetadataManager_Error(
                partitionId,
                replicaId,
                message,
                errorCode.ToHResult());
            break;
        }
        case StateManager:
        {
            StateManagerEventSource::Events->Error(
                partitionId,
                replicaId,
                message,
                errorCode.ToHResult());
            break;
        }
        default:
        {
            ASSERT_IF(true, "PartitionId: {0}: ReplicaId: {1}: ThrowIfNecessary: should not reach the default case.", partitionId, replicaId);
        }
    }

    throw ktl::Exception(Utilities::StatusConverter::Convert(errorCode.ToHResult()));
}

void Helper::TraceErrors(
    __in NTSTATUS status,
    __in Common::Guid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Component component,
    __in std::wstring message)
{
    switch (component)
    {
    case CheckpointFile:
    {
        StateManagerEventSource::Events->CheckpointFileError(
            partitionId,
            replicaId,
            message,
            status);
        break;
    }
    case CheckpointManager:
    {
        StateManagerEventSource::Events->CheckpointManagerError(
            partitionId,
            replicaId,
            message,
            status);
        break;
    }
    case MetadataManager:
    {
        StateManagerEventSource::Events->MetadataManager_Error(
            partitionId,
            replicaId,
            message,
            status);
        break;
    }
    case StateManager:
    {
        StateManagerEventSource::Events->Error(
            partitionId,
            replicaId,
            message,
            status);
        break;
    }
    case NamedOperationDataStream:
    {
        StateManagerEventSource::Events->NamedOperationDataStreamError(
            partitionId,
            replicaId,
            message,
            status);
        break;
    }
    default:
    {
        ASSERT_IF(true, "PartitionId: {0}: ReplicaId: {1}: ThrowIfNecessary: should not reach the default case.", partitionId, replicaId);
    }
    }
}

void Helper::TraceErrors(
    __in Common::ErrorCode errorCode,
    __in Common::Guid partitionId,
    __in FABRIC_REPLICA_ID replicaId,
    __in Component component,
    __in std::wstring message)
{
    switch (component)
    {
    case CheckpointFile:
    {
        StateManagerEventSource::Events->CheckpointFileError(
            partitionId,
            replicaId,
            message,
            errorCode.ToHResult());
        break;
    }
    case CheckpointManager:
    {
        StateManagerEventSource::Events->CheckpointManagerError(
            partitionId,
            replicaId,
            message,
            errorCode.ToHResult());
        break;
    }
    case MetadataManager:
    {
        StateManagerEventSource::Events->MetadataManager_Error(
            partitionId,
            replicaId,
            message,
            errorCode.ToHResult());
        break;
    }
    case StateManager:
    {
        StateManagerEventSource::Events->Error(
            partitionId,
            replicaId,
            message,
            errorCode.ToHResult());
        break;
    }
    default:
    {
        ASSERT_IF(true, "PartitionId: {0}: ReplicaId: {1}: ThrowIfNecessary: should not reach the default case.", partitionId, replicaId);
    }
    }
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0,
    __in Common::VariableArgument const & arg1)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0, arg1);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0,
    __in Common::VariableArgument const & arg1,
    __in Common::VariableArgument const & arg2)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0, arg1, arg2);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0,
    __in Common::VariableArgument const & arg1,
    __in Common::VariableArgument const & arg2,
    __in Common::VariableArgument const & arg3)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0,
    __in Common::VariableArgument const & arg1,
    __in Common::VariableArgument const & arg2,
    __in Common::VariableArgument const & arg3,
    __in Common::VariableArgument const & arg4)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4);

    return message;
}

std::wstring Helper::StringFormat(
    __in Common::StringLiteral format,
    __in Common::VariableArgument const & arg0,
    __in Common::VariableArgument const & arg1,
    __in Common::VariableArgument const & arg2,
    __in Common::VariableArgument const & arg3,
    __in Common::VariableArgument const & arg4,
    __in Common::VariableArgument const & arg5)
{
    std::wstring message;
    Common::StringWriter writer(message);
    writer.Write(format, arg0, arg1, arg2, arg3, arg4, arg5);

    return message;
}

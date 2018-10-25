// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::StateManager;
using namespace Data::Utilities;
using namespace StateManagerTests;

Common::Random TestHelper::random = Common::Random();

KtlSystem* TestHelper::CreateKtlSystem()
{
    KtlSystem* underlyingSystem;
    NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    underlyingSystem->SetStrictAllocationChecks(TRUE);

    return underlyingSystem;
}

PartitionedReplicaId::SPtr TestHelper::CreatePartitionedReplicaId(KAllocator& allocator)
{
    KGuid guid;
    guid.CreateNew();

    return PartitionedReplicaId::Create(guid, random.Next(), allocator);
}

MetadataManager::SPtr TestHelper::CreateMetadataManager(__in KAllocator & allocator)
{
    NTSTATUS status;

    // Create random partition.
    auto partitionedReplicaIdSPtr = CreatePartitionedReplicaId(allocator);

    // Create the SPMM
    MetadataManager::SPtr stateProviderMetadataManager = nullptr;
    status = MetadataManager::Create(*partitionedReplicaIdSPtr, allocator, stateProviderMetadataManager);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return stateProviderMetadataManager;
}

Metadata::SPtr TestHelper::CreateMetadata(__in KUri const & name, __in KAllocator & allocator)
{
    return CreateMetadata(name, true, allocator);
}

Metadata::SPtr TestHelper::CreateMetadata(
    __in KUri const & name, 
    __in bool transientCreate, 
    __in KAllocator & allocator)
{
    LONG64 stateProviderId = random.Next(1, INT_MAX);
    return CreateMetadata(name, transientCreate, stateProviderId, allocator);
}

Metadata::SPtr TestHelper::CreateMetadata(
    __in KUri const & name,
    __in bool transientCreate,
    __in LONG64 stateProviderId,
    __in KAllocator & allocator)
{
    NTSTATUS status;

    // Expected
    KStringView const expectedTypeStringView(TestStateProvider::TypeName);
    KString::SPtr stringSPtr;
    status = KString::Create(stringSPtr, allocator, expectedTypeStringView);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    OperationData::SPtr initParameters = OperationData::Create(allocator);
    KBuffer::SPtr expectedBuffer = nullptr;
    status = KBuffer::Create(16, expectedBuffer, allocator, KTL_TAG_TEST);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    initParameters->Append(*expectedBuffer);

    KGuid partitionId;
    partitionId.CreateNew();
    LONG64 replicaId = random.Next(1, INT_MAX);
    TestStateProvider::SPtr expectedStateProvider2 = nullptr;

    FactoryArguments factoryArguments(
        name, 
        stateProviderId, 
        *stringSPtr, 
        partitionId, 
        replicaId, 
        initParameters.RawPtr());

    status = TestStateProvider::Create(
        factoryArguments,
        allocator, 
        FaultStateProviderType::None,
        expectedStateProvider2);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    LONG64 expectedStateProviderId = stateProviderId;
    LONG64 expectedParentId = random.Next(1, INT_MAX);
    LONG64 expectedCreateLSN = random.Next(1, INT_MAX - 1);
    LONG64 expectedDeleteLSN = random.Next(static_cast<int>(expectedCreateLSN) + 1, INT_MAX);

    MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;

    // Setup
    Metadata::SPtr metadataSPtr = nullptr;
    status = Metadata::Create(
        name,
        *stringSPtr,
        *expectedStateProvider2,
        initParameters.RawPtr(),
        expectedStateProviderId,
        expectedParentId,
        expectedCreateLSN,
        expectedDeleteLSN,
        expectedMetadataMode,
        transientCreate,
        allocator,
        metadataSPtr);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    // Verify the name passing in is the same as metadata name and state provider name.
    ASSERT_IFNOT(metadataSPtr->Name->Compare(name), "TestHelper::CreateMetadata: The Metadata name should be the same as the name passed in.");
    ASSERT_IFNOT(expectedStateProvider2->GetName().Compare(name), "TestHelper::CreateMetadata: The StateProvider name should be the same as the name passed in.");

    return metadataSPtr;
}

NamedOperationData::CSPtr TestHelper::CreateNamedOperationData(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in SerializationMode::Enum mode,
    __in KAllocator & allocator,
    __in_opt Data::Utilities::OperationData const * const operationDataPtr)
{
    NTSTATUS status;

    NamedOperationData::CSPtr result = nullptr;
    status = NamedOperationData::Create(stateProviderId, mode, allocator, operationDataPtr, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

NamedOperationData::CSPtr TestHelper::CreateNamedOperationData(
    __in KAllocator & allocator,
    __in_opt Data::Utilities::OperationData const * const namedOperationDataPtr)
{
    NTSTATUS status;

    NamedOperationData::CSPtr result = nullptr;
    status = NamedOperationData::Create(allocator, namedOperationDataPtr, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

MetadataOperationData::CSPtr TestHelper::CreateMetadataOperationData(
    __in FABRIC_STATE_PROVIDER_ID stateProviderId,
    __in KUri const & stateProviderName,
    __in ApplyType::Enum applyType,
    __in KAllocator & allocator)
{
    NTSTATUS status;

    MetadataOperationData::CSPtr result = nullptr;
    status = MetadataOperationData::Create(stateProviderId, stateProviderName, applyType, allocator, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

MetadataOperationData::CSPtr TestHelper::CreateMetadataOperationData(
    __in KAllocator & allocator,
    __in_opt Data::Utilities::OperationData const * const namedOperationDataPtr)
{
    NTSTATUS status;

    MetadataOperationData::CSPtr result = nullptr;
    status = MetadataOperationData::Create(allocator, namedOperationDataPtr, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

RedoOperationData::CSPtr TestHelper::CreateRedoOperationData(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KString const & typeName,
    __in OperationData const * const initializationParameters,
    __in LONG64 parentStateProviderId,
    __in KAllocator & allocator,
    __in_opt SerializationMode::Enum serializationMode)
{
    NTSTATUS status;

    RedoOperationData::CSPtr result = nullptr;
    status = RedoOperationData::Create(traceId, typeName, initializationParameters, parentStateProviderId, serializationMode, allocator, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

RedoOperationData::CSPtr TestHelper::CreateRedoOperationData(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator,
    __in_opt Data::Utilities::OperationData const * const namedOperationDataPtr)
{
    NTSTATUS status;

    RedoOperationData::CSPtr result = nullptr;
    status = RedoOperationData::Create(traceId, allocator, namedOperationDataPtr, result);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return result;
}

StateManagerLockContext::SPtr TestHelper::CreateLockContext(__in MetadataManager & metadataManager, __in KAllocator& allocator)
{
    NTSTATUS status;

    KUri::CSPtr expectedName;
    status = KUri::Create(KUriView(L"fabric:/sps/sp"), allocator, expectedName);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    const LONG64 expectedStateProviderId = 17;

    // Create
    StateManagerLockContext::SPtr lockContextSPtr = nullptr;
    status = StateManagerLockContext::Create(*expectedName, expectedStateProviderId, metadataManager, allocator, lockContextSPtr);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return lockContextSPtr;
}

TxnReplicator::IRuntimeFolders::SPtr TestHelper::CreateRuntimeFolders(__in KAllocator& allocator)
{
    WCHAR buffer[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buffer);

    TxnReplicator::IRuntimeFolders::SPtr runtimeFolders = Data::TestCommon::TestRuntimeFolders::Create(buffer, allocator);

    return runtimeFolders;
}

IStatefulPartition::SPtr TestHelper::CreateStatefulServicePartition( 
    __in KAllocator & allocator)
{
    return CreateStatefulServicePartition(Common::Guid::NewGuid(), allocator);
}

IStatefulPartition::SPtr TestHelper::CreateStatefulServicePartition(
    __in Common::Guid partitionId,
    __in KAllocator & allocator,
    __in FABRIC_SERVICE_PARTITION_ACCESS_STATUS readStatus,
    __in FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus)
{
    KGuid guid(partitionId.AsGUID());
    auto result = Data::TestCommon::TestStatefulServicePartition::Create(guid, allocator);

    result->SetReadStatus(readStatus);
    result->SetWriteStatus(writeStatus);

    return result.RawPtr();
}

BOOL TestHelper::Equals(
    __in Data::Utilities::OperationData const* const operationData, 
    __in Data::Utilities::OperationData const* const otherOperationData)
{
    if (operationData == nullptr && otherOperationData == nullptr)
    {
        return TRUE;
    }

    if (operationData == nullptr || otherOperationData == nullptr)
    {
        return FALSE;
    }

    if (operationData->BufferCount == otherOperationData->BufferCount)
    {
        for (ULONG32 i = 0; i < operationData->BufferCount; i++)
        {
            auto thisBuffer = (*operationData)[i];
            auto otherBuffer = (*otherOperationData)[i];

            if (thisBuffer == nullptr && otherBuffer == nullptr)
            {
                continue;
            }

            if (thisBuffer == nullptr || otherBuffer == nullptr)
            {
                return FALSE;
            }

            if (thisBuffer->QuerySize() != otherBuffer->QuerySize())
            {
                return FALSE;
            }

            for ( ULONG32 index = 0; index < thisBuffer->QuerySize(); index++)
            {
                KBuffer & tmpBuffer = const_cast<KBuffer &>(*thisBuffer);

                if(tmpBuffer != (*otherBuffer))
                {
                    return FALSE;
                }
            }
        }

        return TRUE;
    }

    return FALSE;
}

KSharedArray<SerializableMetadata::CSPtr>::SPtr TestHelper::CreateSerializableMetadataArray(__in KAllocator & allocator)
{
    KSharedArray<SerializableMetadata::CSPtr>::SPtr outputArraySPtr = _new(CHECKPOINTFILE_TAG, allocator) KSharedArray<SerializableMetadata::CSPtr>();
    CODING_ERROR_ASSERT(outputArraySPtr != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(outputArraySPtr->Status()));
    return outputArraySPtr;
}

KSharedArray<ULONG32>::SPtr TestHelper::CreateRecordBlockSizesArray(__in KAllocator & allocator)
{
    KSharedArray<ULONG32>::SPtr outputArraySPtr = _new(CHECKPOINTFILE_TAG, allocator) KSharedArray<ULONG32>();
    CODING_ERROR_ASSERT(outputArraySPtr != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(outputArraySPtr->Status()));
    return outputArraySPtr;
}

KSharedArray<Metadata::CSPtr>::SPtr TestHelper::CreateMetadataArray(__in KAllocator & allocator)
{
    KSharedArray<Metadata::CSPtr>::SPtr outputArraySPtr = _new(CHECKPOINT_MANAGER_TAG, allocator) KSharedArray<Data::StateManager::Metadata::CSPtr>();
    CODING_ERROR_ASSERT(outputArraySPtr != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(outputArraySPtr->Status()));
    return outputArraySPtr;
}

KSharedArray<KUri::CSPtr>::SPtr TestHelper::CreateStateProviderNameArray(__in KAllocator & allocator)
 {
    KSharedArray<KUri::CSPtr>::SPtr outputArraySPtr = _new(TEST_TAG, allocator) KSharedArray<KUri::CSPtr>();
    CODING_ERROR_ASSERT(outputArraySPtr != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(outputArraySPtr->Status()));
    return outputArraySPtr;
}

void TestHelper::VerifyMetadata(__in_opt Metadata const * const metadata1, __in_opt Metadata const * const metadata2)
{
    CODING_ERROR_ASSERT(metadata1 != nullptr);
    CODING_ERROR_ASSERT(metadata2 != nullptr);
    CODING_ERROR_ASSERT(*(metadata1->Name) == *(metadata2->Name));
    CODING_ERROR_ASSERT(*(metadata1->TypeString) == *(metadata2->TypeString));
    CODING_ERROR_ASSERT(metadata1->CreateLSN == metadata2->CreateLSN);
    CODING_ERROR_ASSERT(metadata1->DeleteLSN == metadata2->DeleteLSN);
    CODING_ERROR_ASSERT(metadata1->MetadataMode == metadata2->MetadataMode);
    CODING_ERROR_ASSERT(metadata1->ParentId == metadata2->ParentId);
    CODING_ERROR_ASSERT(metadata1->StateProviderId == metadata2->StateProviderId);
    CODING_ERROR_ASSERT(metadata1->TransactionId == metadata2->TransactionId);
    CODING_ERROR_ASSERT(metadata1->MetadataMode == metadata2->MetadataMode);
    CODING_ERROR_ASSERT(metadata1->TransientCreate == metadata2->TransientCreate);
    CODING_ERROR_ASSERT(metadata1->TransientDelete == metadata2->TransientDelete);
}

OperationData::SPtr TestHelper::CreateInitParameters(
    __in KAllocator & allocator,
    __in_opt ULONG32 bufferCount,
    __in_opt ULONG32 bufferSize)
{
    OperationData::SPtr initParameters = OperationData::Create(allocator);

    for (ULONG32 i = 0; i < bufferCount; i++)
    {
        KBuffer::SPtr expectedBuffer = nullptr;
        NTSTATUS status = KBuffer::Create(bufferSize, expectedBuffer, allocator, KTL_TAG_TEST);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Create a byte array needs the const array size
        // Set the size to 1024 because the largest buffer we created for test is 512.
        CODING_ERROR_ASSERT(bufferSize < 1024);
        byte temp[1024] = {};

        for (ULONG32 j = 0; j < bufferSize; j++)
        {
            temp[j] = static_cast<byte>(i);
        }

        byte * point = static_cast<byte*>(expectedBuffer->GetBuffer());
        KMemCpySafe(point, bufferSize, temp, bufferSize);

        // If append failed, it will throw status, thus we failed the CreateInitParameters
        try
        {
            initParameters->Append(*expectedBuffer);
        }
        catch (Exception & exception)
        {
            CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_UNSUCCESSFUL);
            CODING_ERROR_ASSERT(false);
        }
    }

    return initParameters;
}

void TestHelper::VerifyInitParameters(
    __in OperationData const & initParameters,
    __in_opt ULONG32 bufferCount,
    __in_opt ULONG32 bufferSize)
{
    CODING_ERROR_ASSERT(bufferCount == initParameters.get_BufferCount());

    for (ULONG32 i = 0; i < bufferCount; i++)
    {
        KBuffer::SPtr buffer = const_cast<KBuffer *>(initParameters[i].RawPtr());

        CODING_ERROR_ASSERT(buffer->QuerySize() == bufferSize);

        byte * point = static_cast<byte*>(buffer->GetBuffer());

        for (ULONG32 j = 0; j < bufferSize; j++)
        {
            CODING_ERROR_ASSERT(point[j] == static_cast<byte>(i));
        }
    }
}

KWString TestHelper::CreateFileName(
    __in KStringView const & name,
    __in KAllocator & allocator,
    __in_opt FileSource fileSource)
{
    wstring currentDirectory = Common::Directory::GetCurrentDirectoryW();

    if (fileSource == FileSource::Managed)
    {
#ifdef PLATFORM_UNIX
        // #10358503: Temporary fix for running StateManager tests on Linux with RunTests
        wstring linuxFileModifier(L"/data_upgrade/5.1/NativeStateManager");
        if (Common::Directory::Exists(currentDirectory + linuxFileModifier))
        {
            currentDirectory += linuxFileModifier;
        }
        else
        {
            currentDirectory = currentDirectory + L"/.." + linuxFileModifier;
        }
#else
        currentDirectory = currentDirectory + L"\\UpgradeTests\\5.1\\NativeStateManager";
#endif
    }

    KString::SPtr path = KPath::CreatePath(currentDirectory.c_str(), allocator);
    KPath::CombineInPlace(*path, name);

    KWString fileName(allocator, *path);
    return fileName;
}

KString::CSPtr TestHelper::CreateFileString(
    __in KAllocator & allocator,
    __in KStringView const & name)
{
    wstring currentDirectory = Common::Directory::GetCurrentDirectoryW();

    KString::SPtr path = KPath::CreatePath(currentDirectory.c_str(), allocator);
    KPath::CombineInPlace(*path, name);

    return Ktl::Move(path.RawPtr());
}

SerializableMetadata::CSPtr TestHelper::CreateSerializableMetadata(
    __in LONG64 expectedId,
    __in KUriView & expectedNameView,
    __in LONG64 expectedParentId,
    __in LONG64 expectedCreateLSN,
    __in LONG64 expectedDeleteLSN,
    __in KAllocator & allocator)
{
    KUri::CSPtr expectedNameCSPtr;
    NTSTATUS status = KUri::Create(expectedNameView, allocator, expectedNameCSPtr);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KString::SPtr expectedTypeCSPtr;
    status = KString::Create(expectedTypeCSPtr, allocator, TestStateProvider::TypeName);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    OperationData::SPtr initParameters = CreateInitParameters(allocator);

    // Setup
    SerializableMetadata::CSPtr serializablemetadataSPtr = nullptr;
    status = SerializableMetadata::Create(
        *expectedNameCSPtr,
        *expectedTypeCSPtr,
        expectedId,
        expectedParentId,
        expectedCreateLSN,
        expectedDeleteLSN,
        MetadataMode::Enum::Active,
        allocator,
        initParameters.RawPtr(),
        serializablemetadataSPtr);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return serializablemetadataSPtr;
}

KUri::CSPtr TestHelper::CreateStateProviderName(
    __in LONG64 stateProviderId,
    __in KAllocator & allocator)
{
    NTSTATUS status;
    BOOLEAN isSuccess = TRUE;

    KString::SPtr rootString = nullptr;;
    status = KString::Create(rootString, allocator, L"fabric:/sp/");
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    KLocalString<20> nameSuffix;
    isSuccess = nameSuffix.FromLONGLONG(stateProviderId);
    CODING_ERROR_ASSERT(isSuccess == TRUE);

    isSuccess = rootString->Concat(nameSuffix);
    CODING_ERROR_ASSERT(isSuccess == TRUE);

    KUri::SPtr output = nullptr;
    status = KUri::Create(*rootString, allocator, output);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    return output.RawPtr();
}

OperationData::SPtr TestHelper::Read(
    __in BinaryReader & reader,
    __in KAllocator & allocator)
{
    // Read the count of buffers in the OperationData, -1 indicates the OperationData is nullptr
    LONG32 bufferCount = 0;
    reader.Read(bufferCount);

    if (bufferCount == -1)
    {
        return nullptr;
    }

    OperationData::SPtr operationData = OperationData::Create(allocator);

    for (LONG32 i = 0; i < bufferCount; i++)
    {
        // Then read the size of the buffer.
        LONG32 bufferSize = 0;
        reader.Read(bufferSize);

        KBuffer::SPtr buffer = nullptr;
        NTSTATUS status = KBuffer::Create(
            bufferSize,
            buffer,
            allocator,
            TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        if (bufferSize == 0)
        {
            operationData->Append(*buffer.RawPtr());
            continue;
        }

        // Read the specified length of buffer.
        reader.Read(bufferSize, buffer);
        operationData->Append(*buffer.RawPtr());
    }

    return operationData;
}

void TestHelper::ReadOperationDatas(
    __in Data::Utilities::BinaryReader & reader,
    __in KAllocator & allocator,
    __out Data::Utilities::OperationData::CSPtr & metadata,
    __out Data::Utilities::OperationData::CSPtr & redo,
    __out Data::Utilities::OperationData::CSPtr & undo)
{
    metadata = Read(reader, allocator).RawPtr();
    redo = Read(reader, allocator).RawPtr();
    undo = Read(reader, allocator).RawPtr();
}

KString::SPtr TestHelper::GetStateManagerWorkDirectory(
    __in LPCWSTR runtimeFolders,
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator)
{
    KString::SPtr replicaFolderPath = KPath::CreatePath(runtimeFolders, allocator);

    KString::SPtr folderName = nullptr;
    NTSTATUS status = KString::Create(folderName, allocator);
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    Common::Guid partitionIdGuid(traceId.PartitionId);
    wstring partitionIdString = partitionIdGuid.ToString('N');

    BOOLEAN boolStatus = folderName->Concat(partitionIdString.c_str());
    CODING_ERROR_ASSERT(boolStatus == TRUE);

    // This can also be a separate folder. However, then we would need to clean both.
    boolStatus = folderName->Concat(L"_");
    CODING_ERROR_ASSERT(boolStatus == TRUE);

    KLocalString<20> replicaIdString;
    boolStatus = replicaIdString.FromLONGLONG(traceId.ReplicaId);
    CODING_ERROR_ASSERT(boolStatus == TRUE);

    boolStatus = folderName->Concat(replicaIdString);
    CODING_ERROR_ASSERT(boolStatus == TRUE);

    KPath::CombineInPlace(*replicaFolderPath, *folderName);

    return replicaFolderPath;
}

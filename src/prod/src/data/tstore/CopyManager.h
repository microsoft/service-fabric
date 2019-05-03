// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define COPY_MANAGER_TAG 'cpMG'

namespace Data
{
    namespace TStore
    {
        class CopyManager
            : public KObject<CopyManager>
            , public KShared<CopyManager>
            , public ICopyManager
        {
            K_FORCE_SHARED(CopyManager)
            K_SHARED_INTERFACE_IMP(ICopyManager)

        public:
            static NTSTATUS Create(
                __in KStringView const & directory,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __in StorePerformanceCountersSPtr & perfCounters,
                __out SPtr & result);

            ktl::Awaitable<void> AddCopyDataAsync(__in OperationData const & data) override;

             __declspec(property(get = get_MetadataTable)) MetadataTable::SPtr MetadataTableSPtr;
             MetadataTable::SPtr get_MetadataTable()
             {
                 return metadataTableSPtr_;
             }

             __declspec(property(get = get_IsCopyCompleted)) bool IsCopyCompleted;
             bool get_IsCopyCompleted() override
             {
                 return copyCompleted_;
             }

             ktl::Awaitable<void> CloseAsync() override;

            static const ULONG32 CopyProtocolVersion = 1;
            static const ULONG32 InvalidCopyProtocolVersion = 0;

        private:
            ktl::Awaitable<ULONG32> ProcessCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessVersionCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessMetadataTableCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessStartKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessWriteKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessEndKeyFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessStartValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessWriteValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessEndValueFileCopyOperationAsync(__in KStringView const & directory, __in KBuffer & data);
            ktl::Awaitable<void> ProcessCompleteCopyOperationAsync(__in KStringView const & directory);

            KString::SPtr CombinePaths(__in KStringView const & directory, __in KStringView const & file);
            ktl::Awaitable<KBlockFile::SPtr> OpenFileAsync(__in KStringView const & filename);
            static ULONG32 GetULONG32(__in KBuffer & buffer, __in ULONG offsetBytes);
            void TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception);

            CopyManager(
                __in KStringView const & directory,
                __in StoreTraceComponent & traceComponent, 
                __in StorePerformanceCountersSPtr & perfCounters);

            bool copyCompleted_;
            KString::SPtr workDirectorySPtr_;
            KString::SPtr currentCopyFileNameSPtr_;
            KBlockFile::SPtr currentCopyFileSPtr_;
            ktl::io::KFileStream::SPtr currentCopyFileStreamSPtr_;
            ULONG32 copyProtocolVersion_;
            ULONG32 fileCount_;
            MetadataTable::SPtr metadataTableSPtr_;
            StoreTraceComponent::SPtr traceComponent_;
            CopyPerformanceCounterWriter perfCounterWriter_;
        };
    }
}

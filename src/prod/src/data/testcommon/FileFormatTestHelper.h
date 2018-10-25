// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define FAKEFILE_TAG 'FAKE'

namespace Data
{
    namespace TestCommon
    {
        class FileFormatTestHelper
        {
        public:
            static ktl::Awaitable<NTSTATUS> CreateBlockFile(
                __in KWString& fileName,
                __in KAllocator& allocator,
                __in ktl::CancellationToken const & cancellationToken,
                __out KBlockFile::SPtr & fakeFileSPtr,
                __out ktl::io::KFileStream::SPtr & fakeStream
            )
            {
                NTSTATUS status;

                status = co_await KBlockFile::CreateSparseFileAsync(
                    fileName,
                    TRUE,
                    KBlockFile::eOpenAlways,
                    fakeFileSPtr,
                    nullptr,
                    allocator,
                    FAKEFILE_TAG);

                if (!NT_SUCCESS(status))
                {
                    co_return status;
                }

                status = ktl::io::KFileStream::Create(fakeStream, allocator, FAKEFILE_TAG);
                if (!NT_SUCCESS(status))
                {
                    co_return status;
                }

                co_return STATUS_SUCCESS;
            }

            static ktl::Awaitable<NTSTATUS> ForceCreateNewBlockFile(
                __in KWString&  fileName,
                __in KAllocator& allocator,
                __in ktl::CancellationToken const & cancellationToken,
                __out KBlockFile::SPtr & fakeFileSPtr,
                __out ktl::io::KFileStream::SPtr & fakeStream
            )
            {
                NTSTATUS status;

                status = co_await KBlockFile::CreateSparseFileAsync(
                    fileName,
                    TRUE,
                    KBlockFile::eCreateAlways,
                    fakeFileSPtr,
                    nullptr,
                    allocator,
                    FAKEFILE_TAG);


                if (!NT_SUCCESS(status))
                {
                    co_return status;
                }

                status = ktl::io::KFileStream::Create(fakeStream, allocator, FAKEFILE_TAG);
                if (!NT_SUCCESS(status))
                {
                    co_return status;
                }

                co_return STATUS_SUCCESS;
            }
        };
    }
}


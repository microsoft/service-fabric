// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#define STREAMPOOL_TAG 'fpCV'

namespace Data
{
   namespace TStore
   {
      //
      // Represents a pool of reusable streams.
      //
      class StreamPool :
         public KObject<StreamPool>,
         public KShared<StreamPool>
      {
         K_FORCE_SHARED(StreamPool)

      public:

         typedef KDelegate<ktl::Awaitable<ktl::io::KFileStream::SPtr>()> StreamFactoryType;

         static NTSTATUS
            Create(
               __in StreamFactoryType streamFactory,
               __in KAllocator& allocator,
               __out StreamPool::SPtr& result,
               __in ULONG32 maxStreams = DefaultMaxStreams);

         //
         // Acquire a re-usable stream for exclusive use.  This will either return an existing
         // stream from the pool, or create a new one if none exist.
         //
         ktl::Awaitable<ktl::io::KFileStream::SPtr> AcquireStreamAsync();

         //
         // Acquire a re-usable stream for exclusive use.  This will either return an existing
         // stream from the pool, or create a new one if none exist.
         //
         ktl::Awaitable<void> StreamPool::ReleaseStreamAsync(__in ktl::io::KFileStream& stream);
         ktl::Awaitable<void> CloseAsync();

         //
         // Exposed for testing only.
         //
         ULONG32 Count();

      private:

         StreamPool(__in StreamPool::StreamFactoryType streamFactory, __in ULONG32 maxStreams);

         static const ULONG32 DefaultMaxStreams = 16;

         StreamFactoryType streamFactory_;
         KArray<ktl::io::KFileStream::SPtr> streams_;
         ULONG32 maxStreams_;
         bool dropNextStream_ = false;
         bool isClosed_ = false;
         KSpinLock lock_;
      };
   }
}

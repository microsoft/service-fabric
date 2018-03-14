// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;

StreamPool::StreamPool(__in StreamPool::StreamFactoryType streamFactory, __in ULONG32 maxStreams)
   :streamFactory_(streamFactory),
   streams_(GetThisAllocator()),
   maxStreams_(maxStreams)
{
}

StreamPool::~StreamPool()
{
}

NTSTATUS StreamPool::Create(
   __in StreamFactoryType streamFactory,
   __in KAllocator& allocator,
   __out StreamPool::SPtr& result,
   __in ULONG32 maxStreams)
{
   result = _new(STREAMPOOL_TAG, allocator) StreamPool(streamFactory, maxStreams);

   if (!result)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   return STATUS_SUCCESS;
}

//
// Acquire a re-usable stream for exclusive use.  This will either return an existing
// stream from the pool, or create a new one if none exist.
//
ktl::Awaitable<ktl::io::KFileStream::SPtr> StreamPool::AcquireStreamAsync()
{
   K_LOCK_BLOCK(lock_)
   {
      ULONG32 count = streams_.Count();
      if (count > 0)
      {
         ULONG32 lastIndex = count - 1;
         auto stream = streams_[lastIndex];
         BOOLEAN result = streams_.Remove(lastIndex);
         ASSERT_IFNOT(result == TRUE, "remove for index {0} failed", lastIndex);
         stream->SetPosition(0);
         co_return stream;
      }
   }

   // Create a new stream.
   ktl::io::KFileStream::SPtr stream = co_await streamFactory_();
   co_return stream;
}

//
// Acquire a re-usable stream for exclusive use.  This will either return an existing
// stream from the pool, or create a new one if none exist.
//
ktl::Awaitable<void> StreamPool::ReleaseStreamAsync(__in io::KFileStream& stream)
{
   io::KFileStream::SPtr streamSPtr = &stream;
   bool shouldClose = false;
   K_LOCK_BLOCK(lock_)
   {
      if (isClosed_)
      {
         shouldClose = true;
      }
      else
      {
         if (streams_.Count() < maxStreams_)
         {
            NTSTATUS status = streams_.Append(streamSPtr);
            ASSERT_IFNOT(NT_SUCCESS(status), "Falied to appen stream");
            dropNextStream_ = (static_cast<ULONG32>(streams_.Count()) == maxStreams_);
         }
         else
         {
            // There are too many streams - drop approximately every other stream.
            // We only drop gradually because, if there was a burst of requests, we should
            // attempt to keep those already created streams around until the load decreases.
            if (!dropNextStream_)
            {
               NTSTATUS status = streams_.Append(streamSPtr);
               ASSERT_IFNOT(NT_SUCCESS(status), "Falied to appen stream");
               dropNextStream_ = true;
            }
            else
            {
               shouldClose = true;
               dropNextStream_ = false;
            }
         }
      }
   }

   if (shouldClose)
   {
      // Close this stream as well.
      co_await streamSPtr->CloseAsync();
   }
}

ktl::Awaitable<void> StreamPool::CloseAsync()
{
   K_LOCK_BLOCK(lock_)
   {
      isClosed_ = true;
   }

   for (ULONG32 i = 0; i < streams_.Count(); i++)
   {
      co_await (streams_[i])->CloseAsync();
   }

   streams_.Clear();
}


//
// Exposed for testing only.
//
ULONG32 StreamPool::Count()
{
   ULONG32 count = 0;
   K_LOCK_BLOCK(lock_)
   {
      count = streams_.Count();
   }

   return count;
}

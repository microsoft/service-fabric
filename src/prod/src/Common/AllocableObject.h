// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // Defines the base class for an object that can be allocated by a custom allocator
    // passed in via operator new.
    //
    class AllocableObject
    {
        DENY_COPY(AllocableObject)

    public:
        AllocableObject() {}
        struct Header
        {
            KAllocator *allocator;
        };

        static void * operator new(size_t nBytes, KAllocator &ktlAllocator)
        {
            Header* mem = static_cast<Header *>(ktlAllocator.Alloc(nBytes + sizeof(Header)));
            mem->allocator = &ktlAllocator;

            // This is aligned to 8byte boundary.
            return static_cast<void *>(mem + 1);
        }

        // Placement overload for delete, called when there are exceptions from constructor.
        static void operator delete(void *ptr, KAllocator &ktlAllocator)
        {
            ktlAllocator.Free(ptr);
        }

        // Non placement overload for delete called to free this object.
        static void operator delete(void *ptr)
        {
            // This is aligned to 8byte boundary.
            Header * base = reinterpret_cast<Header *>((ULONG_PTR)ptr - sizeof(Header));

            base->allocator->Free(static_cast<void *>(base));
        }
    };
}

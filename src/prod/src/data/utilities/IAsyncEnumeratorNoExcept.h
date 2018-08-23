// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        template<typename T>
        interface IAsyncEnumeratorNoExcept : public IDisposable
        {
            K_SHARED_INTERFACE(IAsyncEnumeratorNoExcept)

        public:

            //
            // Advances the enumerator to the next element of the enumerator, and return the value if it is successfully
            //
            // Returns STATUS_SUCCESS if the enumerator successfully advanced to the next element and return it.
            // Returns STATUS_NOT_FOUND if the enumerator passed the end of the collection
            //
            virtual ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out T & result) noexcept = 0;
            
            //
            // Sets the enumerator to its initial position, which is before the first element
            // in the collection.
            //
            virtual NTSTATUS Reset() noexcept = 0;
        };
    }
}

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
        interface IAsyncEnumerator : public IDisposable
        {
            K_SHARED_INTERFACE(IAsyncEnumerator)

        public:
  
            //
            // Gets the current element in the enumerator
            //
            virtual T GetCurrent() = 0;
            
            //
            // Advances the enumerator to the next element of the enumerator.
            //
            // Returns TRUE if the enumerator successfully advanced to the next element
            // Returns FALSE if the enumerator passed the end of the collection
            //
            virtual ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) = 0;
            
            //
            // Sets the enumerator to its initial position, which is before the first element
            // in the collection.
            //
            virtual void Reset() = 0;
        };
    }
}

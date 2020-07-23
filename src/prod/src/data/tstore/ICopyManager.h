//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        interface ICopyManager
        {
            K_SHARED_INTERFACE(ICopyManager)
        public:
            __declspec(property(get = get_IsCopyCompleted)) bool IsCopyCompleted;
            virtual bool get_IsCopyCompleted() = 0;

            virtual ktl::Awaitable<void> AddCopyDataAsync(__in OperationData const & data) = 0;
            virtual ktl::Awaitable<void> CloseAsync() = 0;
        };
    }
}

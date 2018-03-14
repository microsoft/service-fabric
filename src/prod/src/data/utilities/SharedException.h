// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // An Exception class which is also a KShared
        // 
        // NOTE: A KSharedType<Exception> was not used to implement the functionality because of the lack of constructing a KSharedType with a copy constructor of the underlying object
        //
        class SharedException 
            : public KObject<SharedException>
            , public KShared<SharedException>
        {
            K_FORCE_SHARED(SharedException)

        public:

            static SharedException::CSPtr Create(
                __in ktl::Exception const & exception, 
                __in KAllocator & allocator);

            static SharedException::CSPtr Create(__in KAllocator & allocator);

            __declspec(property(get = get_Info)) ktl::Exception Info;
            ktl::Exception get_Info() const
            {
                return exception_;
            }

            __declspec(property(get = get_StackTrace)) Common::StringLiteral StackTrace;
            Common::StringLiteral get_StackTrace() const
            {
                return ToStringLiteral(stackTrace_);
            }
        
        private:

            SharedException(__in ktl::Exception const & exception);

            ktl::Exception exception_;
            KDynStringA stackTrace_;
        };
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // holds a resource so that it can be cleaned up automatically when it goes out of scope
    template<class TResource>
    class ResourceHolder
    {
        DENY_COPY(ResourceHolder)

    public:
        typedef std::function<void(ResourceHolder<TResource>* thisPtr)> CleanupFunction;

        explicit ResourceHolder(TResource const & value, CleanupFunction const & cleanup)
            : value_(value), cleanup_(cleanup)
        {
            ASSERT_IF(cleanup_ == nullptr, "Cleanup function must be specified.");
        }

        explicit ResourceHolder(TResource && value, CleanupFunction const & cleanup)
            : value_(std::move(value)), cleanup_(cleanup)
        {
            ASSERT_IF(cleanup_ == nullptr, "Cleanup function must be specified.");
        }

        ~ResourceHolder()
        {
            cleanup_(this);
        }

        __declspec(property(get=get_Value)) TResource const & Value;
        TResource const & get_Value() const { return value_; }

    private:
        TResource const value_;
        CleanupFunction const cleanup_;
    };
}

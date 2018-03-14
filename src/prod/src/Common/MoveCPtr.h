// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<typename T>
    class MoveCPtr
    {
        MoveCPtr & operator=(MoveCPtr const &);

    public:
        MoveCPtr(ComPointer<T> && ptr) : ptr_(std::move(ptr))
        {
        }

        MoveCPtr(MoveCPtr const & original) : ptr_(std::move(original.ptr_))
        {
        }

        ComPointer<T> TakeSPtr() const
        {
            return std::move(this->ptr_);
        }

        ComPointer<T> const & GetSPtr() const
        {
            return this->ptr_;
        }

        bool HasPtr() const
        {
            return (this->ptr_.GetRawPointer() != nullptr);
        }

    private:
        mutable ComPointer<T> ptr_;
    };
}

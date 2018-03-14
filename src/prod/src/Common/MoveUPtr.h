// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Common
{
    template<typename T, typename D = std::default_delete<T>>
    class MoveUPtr
    {
        MoveUPtr & operator=(MoveUPtr const &);

    public:
        MoveUPtr(std::unique_ptr<T, D> && ptr) : ptr_(std::move(ptr))
        {
        }

        MoveUPtr(MoveUPtr const & original) : ptr_(std::move(original.ptr_))
        {
        }

        T const* operator->() const
        {
            return ptr_.get();
        }

        T * operator->()
        {
            return ptr_.get();
        }

        std::unique_ptr<T, D> TakeUPtr()
        {
            return std::move(this->ptr_);
        }

        bool HasPtr()
        {
            return (this->ptr_.get() != nullptr);
        }

    private:
        mutable std::unique_ptr<T, D> ptr_;
    };
}

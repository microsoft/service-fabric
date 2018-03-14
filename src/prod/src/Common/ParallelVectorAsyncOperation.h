// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<class Tin, class Tout>
    class ParallelVectorAsyncOperation
        : public ParallelAsyncOperationBase<Tin, Tout>
    {
        DENY_COPY(ParallelVectorAsyncOperation)

    public:
        ParallelVectorAsyncOperation(
            std::vector<Tin> const & input,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) :
            ParallelAsyncOperationBase<Tin, Tout>(input.size(), callback, parent),
            input_(input)
        {
        }

        virtual ~ParallelVectorAsyncOperation()
        {
        }

    protected:
        void OnStartInternal(AsyncOperationSPtr const & thisSPtr) override
        {
            for (auto const &item : input_)
            {
                auto operation = OnStartOperation(
                    item,
                    [this, &item](AsyncOperationSPtr const & operation) { this->OperationCompleted(operation, item, false); },
                    thisSPtr);
                this->OperationCompleted(operation, item, true);
            }
        }

        virtual AsyncOperationSPtr OnStartOperation(Tin const & input, AsyncCallback const & callback, AsyncOperationSPtr const & parent)=0;

    private:
        std::vector<Tin> const input_;
    };
}

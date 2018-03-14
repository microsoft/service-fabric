// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template<class Tkey, class Tvalue, class Tout>
    class ParallelKeyValueAsyncOperation
        : public ParallelAsyncOperationBase<Tkey, Tout>
    {
        DENY_COPY(ParallelKeyValueAsyncOperation)

    public:
        ParallelKeyValueAsyncOperation(
            std::map<Tkey, Tvalue> const & input,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & parent) :
            ParallelAsyncOperationBase<Tkey, Tout>(input.size(), callback, parent),
            input_(input)
        {
        }

        virtual ~ParallelKeyValueAsyncOperation()
        {
        }
    
    protected:
        void OnStartInternal(AsyncOperationSPtr const & thisSPtr) override
        {
            for (auto const &kv : input_)
            {
                auto operation = OnStartOperation(
                    kv.first,
                    kv.second,
                    [this, &kv](AsyncOperationSPtr const & operation) { this->OperationCompleted(operation, kv.first, false); },
                    thisSPtr);
                this->OperationCompleted(operation, kv.first, true);
            }
        }

        virtual AsyncOperationSPtr OnStartOperation(Tkey const & inputKey, Tvalue const & inputValue, AsyncCallback const & callback, AsyncOperationSPtr const & parent)=0;

    private:
        std::map<Tkey, Tvalue> const input_;
    };
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::StateChangeAsyncOperation : public Common::AsyncOperation
    {
    public:
        StateChangeAsyncOperation(
            Store::ReplicatedStoreEvent::Enum event,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent)
            : Common::AsyncOperation(callback, parent) 
            , event_(event) 
        {
        }

        __declspec(property(get=get_Event)) Store::ReplicatedStoreEvent::Enum Event;
        Store::ReplicatedStoreEvent::Enum get_Event() const { return event_; }

        void Complete(Store::ReplicatedStoreState::Enum state, Common::ErrorCode const & error)
        {
            state_ = state;
            this->TryComplete(this->shared_from_this(), error);
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation, __out Store::ReplicatedStoreState::Enum & result)
        {
            auto casted = AsyncOperation::End<StateChangeAsyncOperation>(operation);
            if (casted->Error.IsSuccess())
            {
                result = casted->state_;
            }

            return casted->Error;
        }

    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const &)
        {
            // intentional no-op: externally completed by ReplicatedStore::StateMachine
        }

    private:
        Store::ReplicatedStoreEvent::Enum event_;
        Store::ReplicatedStoreState::Enum state_;
    };
}

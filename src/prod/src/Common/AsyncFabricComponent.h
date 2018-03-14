// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncFabricComponent
    {
        DENY_COPY(AsyncFabricComponent)

    public:
        __declspec(property(get=get_FabricComponentState)) FabricComponentState State;

        Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback = 0, 
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());
        Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const & asyncOperation);

        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & callback = 0, 
            Common::AsyncOperationSPtr const & parent = Common::AsyncOperationSPtr());
        Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

        void Abort();

        inline FabricComponentState const & get_FabricComponentState() const
        {
            return state_;
        }

        virtual ~AsyncFabricComponent(void);

    protected:
        AsyncFabricComponent(void);
    
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) = 0;


        virtual Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan,
            Common::AsyncCallback const & , 
            Common::AsyncOperationSPtr const &) = 0;
        virtual Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) = 0;

        virtual void OnAbort() = 0;

        void ThrowIfCreatedOrOpening();

        Common::RwLock & get_StateLock();

    private:
        FabricComponentState state_;

        class OpenAsyncOperation;
        class CloseAsyncOperation;
    };
}

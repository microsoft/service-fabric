// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::LinuxAsyncServiceBaseOperation
        : public Common::AsyncOperation
    {
        DENY_COPY(LinuxAsyncServiceBaseOperation)

    protected:
        LinuxAsyncServiceBaseOperation(
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : Common::AsyncOperation(callback, parent)
        {
        }
     
        //
        // Implement the actual KTL Service Open/Close call here.
        //
        // # get NTSTATUS from this operation 
        virtual bool StartLinuxServiceOperation() = 0;

        void OnStart(__in Common::AsyncOperationSPtr const& thisSPtr);
        virtual void OnCancel();

    private:

        Common::AsyncOperationSPtr thisSPtr_;
    };
}


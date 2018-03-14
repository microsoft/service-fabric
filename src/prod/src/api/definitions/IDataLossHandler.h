// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IDataLossHandler
    {
    public:
        virtual ~IDataLossHandler() {};

        virtual Common::AsyncOperationSPtr BeginOnDataLoss(
            __in Common::AsyncCallback const & callback,
            __in Common::AsyncOperationSPtr const & state) = 0;

        virtual Common::ErrorCode EndOnDataLoss(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isStateChanged) = 0;
    };
}

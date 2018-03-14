// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IOperationDataStream
    {
        Common::AsyncOperationSPtr BeginGetNext(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state);

        Common::ErrorCode EndGetNext(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out bool & isLast,
            __out IOperationDataSPtr & operation);
    };
}

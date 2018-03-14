// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{   
    using namespace Common;
    using namespace Transport;

    EntreeService::AdminRequestAsyncOperationBase::AdminRequestAsyncOperationBase(
        __in GatewayProperties & properties,
        MessageUPtr && receivedMessage,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : RequestAsyncOperationBase (properties, std::move(receivedMessage), timeout, callback, parent)
    {
    }    

    void EntreeService::AdminRequestAsyncOperationBase::OnRetry(AsyncOperationSPtr const &)
    {
    }
}

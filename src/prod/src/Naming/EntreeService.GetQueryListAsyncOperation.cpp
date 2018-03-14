// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Transport;
    using namespace Query;

     EntreeService::GetQueryListAsyncOperation::GetQueryListAsyncOperation(        
        Query::QueryGatewayUPtr & queryGateway,
        TimeSpan timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
      : TimedAsyncOperation (timeout, callback, parent)
      , queryGateway_(queryGateway)      
      , reply_(nullptr)
    {
    }

    void EntreeService::GetQueryListAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        reply_ = Common::make_unique<Transport::Message>(ServiceModel::QueryResult(queryGateway_->GetQueryList()));
        TryComplete(thisSPtr, ErrorCode::Success());
    }

    ErrorCode EntreeService::GetQueryListAsyncOperation::End(AsyncOperationSPtr operation, MessageUPtr & reply)
    {
        auto casted = AsyncOperation::End<GetQueryListAsyncOperation>(operation);
        auto error = casted->Error;
        
        if (error.IsSuccess())
        {
            reply = std::move(casted->reply_);
        }

        return error;
    }
}

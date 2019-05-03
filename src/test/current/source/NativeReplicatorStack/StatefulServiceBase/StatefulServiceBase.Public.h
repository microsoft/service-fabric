// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

// Base dependencies
#include "../../prod/src/data/txnreplicator/TransactionalReplicator.Public.h"

namespace TXRStatefulServiceBase
{
    using ::_delete;
}

// HttpServer specific dependencies
#include "../../prod/src/Management/HttpTransport/HttpTransport.Server.h"

#include "ComAsyncOperationCallbackHelper.h"
#include "Helpers.h"
#include "StatefulServiceBase.h"
#include "Factory.h"
#include "ComFactory.h"

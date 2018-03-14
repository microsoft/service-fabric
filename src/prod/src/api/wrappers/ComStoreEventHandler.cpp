// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Api;

// ********************************************************************************************************************
// ComStoreEventHandler::ComStoreEventHandler Implementation
//

ComStoreEventHandler::ComStoreEventHandler(IStoreEventHandlerPtr const & impl)
    : IFabricStoreEventHandler(),
    ComUnknownBase(),
    impl_(impl)
{
}

ComStoreEventHandler::~ComStoreEventHandler()
{
}

void ComStoreEventHandler::OnDataLoss(void)
{
    impl_->OnDataLoss();
}

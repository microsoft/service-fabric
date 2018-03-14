// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace Management::ClusterManager;

GlobalWString ClusterManagerMessage::OperationSuccessAction = make_global<wstring>(L"OperationSuccessAction");

void ClusterManagerMessage::SetHeaders(Message & message, wstring const & action)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(Actor::CM));
    message.Headers.Add(FabricActivityHeader(Guid::NewGuid()));
}

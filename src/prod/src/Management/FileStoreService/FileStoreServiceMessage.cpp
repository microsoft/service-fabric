// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ServiceModel;
using namespace SystemServices;
using namespace Management::FileStoreService;

void FileStoreServiceMessage::SetHeaders(Message & message, wstring const & action, ActivityId const & activityId)
{
    message.Headers.Add(ActionHeader(action));
    message.Headers.Add(ActorHeader(Actor::FileStoreService));
    message.Headers.Add(FabricActivityHeader(activityId));
}

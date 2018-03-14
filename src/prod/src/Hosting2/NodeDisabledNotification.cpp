// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

NodeDisabledNotification::NodeDisabledNotification() 
    : taskId_()
{
}

NodeDisabledNotification::NodeDisabledNotification(wstring const & taskId)
    : taskId_(taskId)
{
}

void NodeDisabledNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("NodeDisabledNotification { ");
    w.Write("TaskId = {0}", taskId_);
    w.Write("}");
}

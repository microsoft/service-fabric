// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

NodeEnabledNotification::NodeEnabledNotification()
    : error_(ErrorCodeValue::Success)
{
}

NodeEnabledNotification::NodeEnabledNotification(Common::ErrorCode error)
    : error_(error)
{
}

void NodeEnabledNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("NodeEnabledNotification { ");
    w.Write("Error = {0}", error_);
    w.Write("}");
}

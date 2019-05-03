// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::NetworkInventoryManager;

const std::wstring NetworkErrorCodeResponseMessage::ActionName = L"NetworkErrorCodeResponseMessage";

void NetworkErrorCodeResponseMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << " " << sequenceNumber_ << " : errorCode: " << errorCode_;
}

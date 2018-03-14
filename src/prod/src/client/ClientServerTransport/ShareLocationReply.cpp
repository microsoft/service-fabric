// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

ShareLocationReply::ShareLocationReply()
    : shareLocation_()    
{
}

ShareLocationReply::ShareLocationReply(wstring const & shareLocation)
    : shareLocation_(shareLocation)
{
}

void ShareLocationReply::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ShareLocationReply{ShareLocation={0}}", shareLocation_);
}

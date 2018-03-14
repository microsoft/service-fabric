// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

FabricHostOperationReply::FabricHostOperationReply() 
    : error_(),
    data_()
{
}

FabricHostOperationReply::FabricHostOperationReply(
    ErrorCode const & error,
    wstring const & errorMessage,
    wstring const & data)
    : error_(error),
    errorMessage_(errorMessage),
    data_(data)
{
}

void FabricHostOperationReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FabricHostOperationReply { ");
    w.Write("Error = {0}", error_);
    w.Write("Error message = {0}", errorMessage_);
    w.Write("Data = {0}", data_);
    w.Write("}");
}

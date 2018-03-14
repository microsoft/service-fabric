// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

DeleteUploadSessionRequest::DeleteUploadSessionRequest()
: sessionId_()
{
}

DeleteUploadSessionRequest::DeleteUploadSessionRequest(Common::Guid const & sessionId)
: sessionId_(sessionId)
{
}

void DeleteUploadSessionRequest::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("DeleteUploadSessionRequest{SessionId={0}}", this->sessionId_);
}

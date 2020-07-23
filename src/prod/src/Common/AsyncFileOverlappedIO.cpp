// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include"stdafx.h"
using namespace Common;

_Use_decl_annotations_
ReadFileAsyncOverlapped::ReadFileAsyncOverlapped(AsyncFile::ReadFileAsyncOperation & readFile) : readFile_(readFile)
{
}

void ReadFileAsyncOverlapped::OnComplete(PTP_CALLBACK_INSTANCE pci,ErrorCode const & result, ULONG_PTR bytesTransfered)
{
    readFile_.ReadFileComplete(pci, result, bytesTransfered);
}

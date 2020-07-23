// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class ReadFileAsyncOverlapped : public OverlappedIo
    {
    public:
        ReadFileAsyncOverlapped(__in AsyncFile::ReadFileAsyncOperation & readFile);

    private:
        virtual void OnComplete(PTP_CALLBACK_INSTANCE pci, Common::ErrorCode const & error, ULONG_PTR bytesTransfered) override;
        AsyncFile::ReadFileAsyncOperation & readFile_;
    };
}

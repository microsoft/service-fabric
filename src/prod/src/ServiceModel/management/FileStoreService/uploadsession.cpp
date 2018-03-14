// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

UploadSession::UploadSession()
: uploadSessions_()
{
}

UploadSession::UploadSession(
    std::vector<UploadSessionInfo> const & uploadSessions)
    : uploadSessions_(uploadSessions)
{
}

UploadSession& UploadSession::operator = (UploadSession const & other)
{
    if (this != &other)
    {
        this->uploadSessions_ = other.uploadSessions_;
    }

    return *this;
}

UploadSession& UploadSession::operator = (std::vector<UploadSessionInfo> const & session)
{
    this->uploadSessions_ = session;
    return *this;
}

UploadSession& UploadSession::operator = (UploadSession && other)
{
    if (this != &other)
    {
        this->uploadSessions_ = move(other.uploadSessions_);
    }

    return *this;
}

UploadSession& UploadSession::operator = (std::vector<UploadSessionInfo> && session)
{
    this->uploadSessions_ = std::move(session);
    return *this;
}

void UploadSession::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UploadSessionCount={0}", this->uploadSessions_.size());
}

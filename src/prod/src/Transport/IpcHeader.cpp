// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;

IpcHeader::IpcHeader() : fromProcessId_(0)
{ 
}

IpcHeader::IpcHeader(std::wstring const & from, DWORD fromProcessId) : from_(from), fromProcessId_(fromProcessId)
{
}

IpcHeader::IpcHeader(IpcHeader && rhs) : from_(std::move(rhs.from_)), fromProcessId_(rhs.fromProcessId_)
{
}

IpcHeader & IpcHeader::operator=(IpcHeader && rhs)
{
    if (this != &rhs)
    {
        this->from_ = std::move(rhs.from_);
        this->fromProcessId_ = std::move(rhs.fromProcessId_);
    }

    return *this;
}

void IpcHeader::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("IpcHeader({0}, {1})", from_, fromProcessId_);
}

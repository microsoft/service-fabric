// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

FabricRuntimeContext::FabricRuntimeContext()
    : runtimeId_(),
    hostContext_(),
    codeContext_()
{
}

FabricRuntimeContext::FabricRuntimeContext(
    FabricRuntimeContext const & other)
    : runtimeId_(other.runtimeId_),
    hostContext_(other.hostContext_),
    codeContext_(other.codeContext_)
{
}

FabricRuntimeContext::FabricRuntimeContext(
    FabricRuntimeContext && other)
    : runtimeId_(move(other.runtimeId_)),
    hostContext_(move(other.hostContext_)),
    codeContext_(move(other.codeContext_))
{
}

FabricRuntimeContext::FabricRuntimeContext(
    ApplicationHostContext const & hostContext,
    CodePackageContext const & codeContext)
    : runtimeId_(Guid::NewGuid().ToString()),
    hostContext_(hostContext),
    codeContext_(codeContext)
{
}

FabricRuntimeContext::FabricRuntimeContext(
    wstring const & runtimeId,
    ApplicationHostContext const & hostContext,
    CodePackageContext const & codeContext)
    : runtimeId_(runtimeId),
    hostContext_(hostContext),
    codeContext_(codeContext)
{
}

 FabricRuntimeContext const & FabricRuntimeContext::operator = (FabricRuntimeContext const & other)
 {
     if (this != &other)
     {
         this->runtimeId_ = other.runtimeId_;
         this->hostContext_ = other.hostContext_;
         this->codeContext_ = other.codeContext_;
     }

     return *this;
 }

 FabricRuntimeContext const & FabricRuntimeContext::operator = (FabricRuntimeContext && other)
 {
     if (this != &other)
     {
         this->runtimeId_ = move(other.runtimeId_);
         this->hostContext_ = move(other.hostContext_);
         this->codeContext_ = move(other.codeContext_);
     }

     return *this;
 }

bool FabricRuntimeContext::operator == (FabricRuntimeContext const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->runtimeId_, other.runtimeId_);
    if (!equals) { return equals; }

    equals = (this->hostContext_ == other.hostContext_);
    if (!equals) { return equals; }

    equals = (this->codeContext_ == other.codeContext_);
    if (!equals) { return equals; }

    return equals;
}

bool FabricRuntimeContext::operator != (FabricRuntimeContext const & other) const
{
    return !(*this == other);
}

void FabricRuntimeContext::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("FabricRuntimeContext { ");
    w.Write("RuntimeId = {0}, ", RuntimeId);
    w.Write("HostContext = {0}", HostContext);
    w.Write("CodeContext = {0}", CodeContext);
    w.Write("}");
}

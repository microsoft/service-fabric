// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

StartRegisterApplicationHostRequest::StartRegisterApplicationHostRequest() 
    : id_(),
    type_(),
    processId_(0),
    isContainerHost_(false),
    timeout_()
{
}

StartRegisterApplicationHostRequest::StartRegisterApplicationHostRequest(
    wstring const & applicationHostId,
    ApplicationHostType::Enum applicationHostType,
    DWORD applicationHostProcessId,
    bool isContainerHost,
    TimeSpan timeout)
    : id_(applicationHostId),
    type_(applicationHostType),
    processId_(applicationHostProcessId),
    isContainerHost_(isContainerHost),
    timeout_(timeout)
{
}

void StartRegisterApplicationHostRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("StartRegisterApplicationHostRequest { ");
    w.Write("Id = {0}, ", Id);
    w.Write("Type = {0}, ", Type);
    w.Write("ProcessId = {0}, ", ProcessId);
    w.Write("IsContainerHost = {0}, ", IsContainerHost);
    w.Write("Timeout = {0}", Timeout);
    w.Write("}");
}

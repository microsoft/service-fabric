// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerApiExecutionArgs::ContainerApiExecutionArgs()
    : ContainerName()
    , HttpVerb()
    , UriPath()
    , ContentType()
    , RequestBody()
{
}

ContainerApiExecutionArgs::ContainerApiExecutionArgs(
    wstring const & containerName,
    wstring const & httpVerb,
    wstring const & uriPath,
    wstring const & contentType,
    wstring const & requestBody)
    : ContainerName(containerName)
    , HttpVerb(httpVerb)
    , UriPath(uriPath)
    , ContentType(contentType)
    , RequestBody(requestBody)
{
}

ErrorCode ContainerApiExecutionArgs::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_API_EXECUTION_ARGS & apiExecutionArgs) const
{
    apiExecutionArgs.ContainerName = heap.AddString(this->ContainerName);
    apiExecutionArgs.HttpVerb = heap.AddString(this->HttpVerb);
    apiExecutionArgs.UriPath = heap.AddString(this->UriPath);
    apiExecutionArgs.ContentType = heap.AddString(this->ContentType);
    apiExecutionArgs.RequestBody = heap.AddString(this->RequestBody);

    apiExecutionArgs.Reserved = nullptr;

    return ErrorCode::Success();
}

void ContainerApiExecutionArgs::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerApiExecutionArgs { ");
    w.Write("ContainerName = {0}", ContainerName);
    w.Write("HttpVerb = {0}", HttpVerb);
    w.Write("UriPath = {0}", UriPath);
    w.Write("ContentType = {0}, ", ContentType);
    w.Write("RequestBody = {0}", RequestBody);
    w.Write("}");
}


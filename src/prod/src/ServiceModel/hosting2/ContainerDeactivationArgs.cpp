// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerDeactivationArgs::ContainerDeactivationArgs()
    : ContainerName()
    , ConfiguredForAutoRemove()
    , IsContainerRoot()
    , CgroupName()
    , IsGracefulDeactivation()
{
}

ContainerDeactivationArgs::ContainerDeactivationArgs(
    wstring const & containerName,
    bool configuredForAutoRemove,
    bool isContainerRoot,
    wstring const & cgroupName,
    bool isGracefulDeactivation)
    : ContainerName(containerName)
    , ConfiguredForAutoRemove(configuredForAutoRemove)
    , IsContainerRoot(isContainerRoot)
    , CgroupName(cgroupName)
    , IsGracefulDeactivation(isGracefulDeactivation)
{
}

ErrorCode ContainerDeactivationArgs::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_CONTAINER_DEACTIVATION_ARGS & fabricDeactivationArgs) const
{
    fabricDeactivationArgs.ContainerName = heap.AddString(this->ContainerName);
    fabricDeactivationArgs.ConfiguredForAutoRemove = this->ConfiguredForAutoRemove;
    fabricDeactivationArgs.IsContainerRoot = this->IsContainerRoot;
    fabricDeactivationArgs.CgroupName = heap.AddString(this->CgroupName);
    fabricDeactivationArgs.IsGracefulDeactivation = this->IsGracefulDeactivation;
    
    fabricDeactivationArgs.Reserved = nullptr;

    return ErrorCode::Success();
}

void ContainerDeactivationArgs::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerActivationArgs { ");
    w.Write("ContainerName = {0}", ContainerName);
    w.Write("ConfiguredForAutoRemove = {0}", ConfiguredForAutoRemove);
    w.Write("IsContainerRoot = {0}", IsContainerRoot);
    w.Write("CgroupName = {0}, ", CgroupName);
    w.Write("IsGracefulDeactivation = {0}", IsGracefulDeactivation);
    w.Write("}");
}

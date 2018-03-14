// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;

bool TestServiceInfo::operator == (TestServiceInfo const& other ) const
{
    bool isRoleValid = true;

    if(IsStateful)
    {
        isRoleValid = (replicaRole_ == other.replicaRole_);
    }
    else
    {
        isRoleValid = (replicaRole_ == other.replicaRole_);
        isRoleValid = (replicaRole_ == ::FABRIC_REPLICA_ROLE_NONE);
    }

    return ((applicationId_.compare(other.applicationId_) == 0) 
        && (serviceName_.compare(other.serviceName_) == 0) 
        && (serviceType_.compare(other.serviceType_) == 0)
        && (partitionId_.compare(other.partitionId_) == 0)
        && (serviceLocation_.compare(other.serviceLocation_) == 0)
        && (isStateful_ == other.isStateful_)
        && isRoleValid);
}


void TestServiceInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("TestServiceInfo {");
    w.Write("ApplicationId = {0}, ", applicationId_);
    w.Write("ServiceName = {0}, ", serviceName_);
    w.Write("ServiceType = {0}, ", serviceType_);
    w.Write("PartitionId = {0}, ", partitionId_);
    w.Write("ServiceLocation = {0}, ", serviceLocation_);
    w.Write("IsStateful = {0}, ", isStateful_);
    w.Write("StatefulServiceRole = {0}", replicaRole_);
    w.Write("}");
}

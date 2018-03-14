// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::FaultAnalysisService;

StringLiteral const TraceComponent("CancelTestCommandDescription");

CancelTestCommandDescription::CancelTestCommandDescription() 
    : operationId_()
    , force_(false)
{ 
}

CancelTestCommandDescription::CancelTestCommandDescription(Guid &operationId, bool force)
: operationId_(operationId)
, force_(force)
{
}


CancelTestCommandDescription::CancelTestCommandDescription(CancelTestCommandDescription const & other)
: operationId_(other.Id)
, force_(other.Force)
{
}

CancelTestCommandDescription::CancelTestCommandDescription(CancelTestCommandDescription && other) 
    : operationId_(move(other.operationId_))
    , force_(move(other.force_))
{ 
}

bool CancelTestCommandDescription::operator == (CancelTestCommandDescription const & other) const
{
    if (operationId_ != other.operationId_) { return false; }

    if (force_ != other.force_) { return false; }

    return true;
}

bool CancelTestCommandDescription::operator != (CancelTestCommandDescription const & other) const
{
    return !(*this == other);
}

void CancelTestCommandDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
   w << "[ " << operationId_;  
   
   w << ", " << force_ << " ]";
}

Common::ErrorCode CancelTestCommandDescription::FromPublicApi(
    FABRIC_CANCEL_TEST_COMMAND_DESCRIPTION const & publicDescription)
{
    operationId_ = Guid(publicDescription.OperationId);

    force_ = publicDescription.Force ? true : false;
    
    return ErrorCodeValue::Success;
}

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

StringLiteral const TraceComponent("StartChaosDescription");

StartChaosDescription::StartChaosDescription()
: chaosParametersUPtr_()
{
}

StartChaosDescription::StartChaosDescription(StartChaosDescription && other)
: chaosParametersUPtr_(move(other.chaosParametersUPtr_))
{
}

StartChaosDescription & StartChaosDescription::operator =(StartChaosDescription && other)
{
    if (this != &other)
    {
        chaosParametersUPtr_ = move(other.chaosParametersUPtr_);
    }

    return *this;
}

StartChaosDescription::StartChaosDescription(std::unique_ptr<ChaosParameters> && chaosParameters)
: chaosParametersUPtr_(move(chaosParameters))
{
}

Common::ErrorCode StartChaosDescription::FromPublicApi(
    FABRIC_START_CHAOS_DESCRIPTION const & publicDescription)
{
    ChaosParameters chaosParameters;
    auto error = chaosParameters.FromPublicApi(*publicDescription.ChaosParameters);

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "StartChaosDescription::FromPublicApi/Failed at chaosParameters.FromPublicApi");
        return error;
    }

    chaosParametersUPtr_ = make_unique<ChaosParameters>(move(chaosParameters));

    return ErrorCodeValue::Success;
}

void StartChaosDescription::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_START_CHAOS_DESCRIPTION & result) const
{
    auto publicChaosParametersPtr = heap.AddItem<FABRIC_CHAOS_PARAMETERS>();
    chaosParametersUPtr_->ToPublicApi(heap, *publicChaosParametersPtr);
    result.ChaosParameters = publicChaosParametersPtr.GetRawPointer();
}

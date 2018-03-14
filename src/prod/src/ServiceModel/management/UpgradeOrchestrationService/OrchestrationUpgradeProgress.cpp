// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceComponent("OrchestrationUpgradeProgress");

OrchestrationUpgradeProgress::OrchestrationUpgradeProgress()
    : state_(OrchestrationUpgradeState::Invalid),
    progressStatus_(0),
	configVersion_(),
    details_()
{ 
}

OrchestrationUpgradeProgress::OrchestrationUpgradeProgress(OrchestrationUpgradeProgress && other)
    : state_(move(other.state_)),
    progressStatus_(move(other.progressStatus_)),
	configVersion_(move(other.configVersion_)),
    details_(move(other.details_))
{ 
}

OrchestrationUpgradeProgress & OrchestrationUpgradeProgress::operator=(OrchestrationUpgradeProgress && other)
{
    if (this != &other)
    {
        state_ = move(other.state_);
        progressStatus_ = move(other.progressStatus_);
		configVersion_ = move(other.configVersion_);
        details_ = move(other.details_);
    }

    return *this;
}

OrchestrationUpgradeProgress::OrchestrationUpgradeProgress(
    OrchestrationUpgradeState::Enum state,
    uint32 progressStatus,
	const std::wstring &configVersion,
    const std::wstring &details)
    : state_(state),
    progressStatus_(progressStatus),
	configVersion_(configVersion),
    details_(details)
{ 
}

Common::ErrorCode OrchestrationUpgradeProgress::FromPublicApi(__in FABRIC_ORCHESTRATION_UPGRADE_PROGRESS const & publicProgress)
{
	progressStatus_ = publicProgress.ProgressStatus;

	Common::ErrorCode result = OrchestrationUpgradeState::FromPublicApi(publicProgress.UpgradeState, state_);
	if (!result.IsSuccess())
	{
		return result;
	}

	if (publicProgress.Reserved != NULL)
	{
		FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1* ex1 = static_cast<FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1*>(publicProgress.Reserved);
		HRESULT hr = StringUtility::LpcwstrToWstring2(ex1->ConfigVersion, true, configVersion_).ToHResult();
		if (FAILED(hr))
		{
			Trace.WriteError(TraceComponent, "OrchestrationUpgradeProgress::FromPublicApi/Failed to parse ConfigVersion: {0}", hr);
			return ErrorCode::FromHResult(hr);
		}

        if (ex1->Reserved != NULL)
        {
            FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2* ex2 = static_cast<FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2*>(ex1->Reserved);
            hr = StringUtility::LpcwstrToWstring2(ex2->Details, true, details_).ToHResult();
            if (FAILED(hr))
            {
                Trace.WriteError(TraceComponent, "OrchestrationUpgradeProgress::FromPublicApi/Failed to parse Details: {0}", hr);
                return ErrorCode::FromHResult(hr);
            }
        }
	}

	return ErrorCodeValue::Success;
}

void OrchestrationUpgradeProgress::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_ORCHESTRATION_UPGRADE_PROGRESS & progress) const
{
	progress.UpgradeState = OrchestrationUpgradeState::ToPublicApi(state_);
	progress.ProgressStatus = progressStatus_;

    auto ex1 = heap.AddItem<FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX1>();
    ex1->ConfigVersion = heap.AddString(configVersion_);

    auto ex2 = heap.AddItem<FABRIC_ORCHESTRATION_UPGRADE_PROGRESS_EX2>();
    ex2->Details = heap.AddString(details_);
    ex1->Reserved = ex2.GetRawPointer();

    progress.Reserved = ex1.GetRawPointer();
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceComponent("UpgradeOrchestrationServiceState");

UpgradeOrchestrationServiceState::UpgradeOrchestrationServiceState()
	: currentCodeVersion_(),
	currentManifestVersion_(),
	targetCodeVersion_(),
	targetManifestVersion_(),
	pendingUpgradeType_()
{ 
}

UpgradeOrchestrationServiceState::UpgradeOrchestrationServiceState(UpgradeOrchestrationServiceState && other)
    : currentCodeVersion_(move(other.currentCodeVersion_)),
	currentManifestVersion_(move(other.currentManifestVersion_)),
	targetCodeVersion_(move(other.targetCodeVersion_)),
	targetManifestVersion_(move(other.targetManifestVersion_)),
	pendingUpgradeType_(move(other.pendingUpgradeType_))
{ 
}

UpgradeOrchestrationServiceState & UpgradeOrchestrationServiceState::operator=(UpgradeOrchestrationServiceState && other)
{
    if (this != &other)
    {
		currentCodeVersion_ = move(other.currentCodeVersion_),
		currentManifestVersion_ = move(other.currentManifestVersion_),
		targetCodeVersion_ = move(other.targetCodeVersion_),
		targetManifestVersion_ = move(other.targetManifestVersion_),
		pendingUpgradeType_ = move(other.pendingUpgradeType_);
    }

    return *this;
}

UpgradeOrchestrationServiceState::UpgradeOrchestrationServiceState(
	const std::wstring &currentCodeVersion,
	const std::wstring &currentManifestVersion,
	const std::wstring &targetCodeVersion,
	const std::wstring &targetManifestVersion,
	const std::wstring &pendingUpgradeType)
	: currentCodeVersion_(move(currentCodeVersion)),
	currentManifestVersion_(move(currentManifestVersion)),
	targetCodeVersion_(move(targetCodeVersion)),
	targetManifestVersion_(move(targetManifestVersion)),
	pendingUpgradeType_(move(pendingUpgradeType))
{ 
}

void UpgradeOrchestrationServiceState::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_UPGRADE_ORCHESTRATION_SERVICE_STATE & state) const
{
	state.CurrentCodeVersion = heap.AddString(currentCodeVersion_);
	state.CurrentManifestVersion = heap.AddString(currentManifestVersion_);
	state.TargetCodeVersion = heap.AddString(targetCodeVersion_);
	state.TargetManifestVersion = heap.AddString(targetManifestVersion_);
	state.PendingUpgradeType = heap.AddString(pendingUpgradeType_);
}

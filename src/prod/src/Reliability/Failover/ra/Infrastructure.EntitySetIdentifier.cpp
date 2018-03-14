// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

const Global<EntitySetIdentifier> EntitySetIdentifier::ReconfigurationMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::ReconfigurationMessageRetry);
const Global<EntitySetIdentifier> EntitySetIdentifier::StateCleanup = make_global<EntitySetIdentifier>(EntitySetName::StateCleanup);
const Global<EntitySetIdentifier> EntitySetIdentifier::ReplicaCloseMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::ReplicaCloseMessageRetry);
const Global<EntitySetIdentifier> EntitySetIdentifier::ReplicaOpenMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::ReplicaOpenMessageRetry);
const Global<EntitySetIdentifier> EntitySetIdentifier::UpdateServiceDescriptionMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::UpdateServiceDescriptionMessageRetry);
const Global<EntitySetIdentifier> EntitySetIdentifier::FmMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::FailoverManagerMessageRetry, FailoverManagerId(false));
const Global<EntitySetIdentifier> EntitySetIdentifier::FmmMessageRetry = make_global<EntitySetIdentifier>(EntitySetName::FailoverManagerMessageRetry, FailoverManagerId(true));;
const Global<EntitySetIdentifier> EntitySetIdentifier::FmLastReplicaUp = make_global<EntitySetIdentifier>(EntitySetName::ReplicaUploadPending, FailoverManagerId(false));;
const Global<EntitySetIdentifier> EntitySetIdentifier::FmmLastReplicaUp = make_global<EntitySetIdentifier>(EntitySetName::ReplicaUploadPending, FailoverManagerId(true));;

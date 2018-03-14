// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

#define DEFINE_JOB_ITEM_TRACE_PARAMETERS(name, frequency, failFast) Global<JobItemDescription> JobItemDescription::name = make_global<JobItemDescription>(JobItemName::name, JobItemTraceFrequency::frequency, failFast);

DEFINE_JOB_ITEM_TRACE_PARAMETERS(MessageProcessing,                                 Always,                 false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ReconfigurationMessageRetry,                       OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ReplicaCloseMessageRetry,                          OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ReplicaOpenMessageRetry,                           OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(NodeUpdateService,                                 OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(NodeActivationDeactivation,                        OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(Query,                                             OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ReplicaUpReply,                                    OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(StateCleanup,                                      OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(FabricUpgradeRollback,                             OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(FabricUpgradeUpdateEntity,                         OnSuccessfulProcess,    true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ApplicationUpgradeEnumerateFTs,                    OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ApplicationUpgradeUpdateVersionsAndCloseIfNeeded,  OnSuccessfulProcess,    true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ApplicationUpgradeFinalizeFT,                      OnSuccessfulProcess,    true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ApplicationUpgradeReplicaDownCompletionCheck,      OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(AppHostClosed,                                     OnSuccessfulProcess,    true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(RuntimeClosed,                                     OnSuccessfulProcess,    true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ServiceTypeRegistered,                             OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(GetLfum,                                           Never,                  false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(GenerationUpdate,                                  Always,                 false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(NodeUpAck,                                         Always,                 true);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(RAPQuery,                                          OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(ClientReportFault,                                 Always,                 false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(UpdateServiceDescriptionMessageRetry,              OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(CheckReplicaCloseProgressJobItem,                  OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(UpdateStateOnLFUMLoad,                             Never,                  false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(CloseFailoverUnitJobItem,                          Always,                 true);             
DEFINE_JOB_ITEM_TRACE_PARAMETERS(UploadForReplicaDiscovery,                         OnSuccessfulProcess,    false);
DEFINE_JOB_ITEM_TRACE_PARAMETERS(UpdateAfterFMMessageSend,                          Never,                  false);

JobItemDescription::JobItemDescription(JobItemName::Enum name, JobItemTraceFrequency::Enum frequency, bool shouldFailFastOnCommitFailure)
: name_(name),
  frequency_(frequency),
  shouldFailFastOnCommitFailure_(shouldFailFastOnCommitFailure)
{
}

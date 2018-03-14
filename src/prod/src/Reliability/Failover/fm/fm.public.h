// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/Common.h"
#include "ServiceModel/ServiceModel.h"

#include "Federation/Federation.h"
#include "Transport/Transport.h"

#include "Store/store.h"

#include "Reliability/Failover/FailoverPointers.h"

#include "Reliability/Failover/fm/BasicFailoverReplyMessageBody.h"
#include "Reliability/Failover/fm/BasicFailoverRequestMessageBody.h"
#include "Reliability/Failover/fm/UpgradeRequestMessageBody.h"
#include "Reliability/Failover/fm/UpgradeReplyMessageBody.h"
#include "Reliability/Failover/fm/UpgradeApplicationReplyMessageBody.h"
#include "Reliability/Failover/fm/UpgradeFabricReplyMessageBody.h"
#include "Reliability/Failover/fm/NodeStateRemovedRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverPartitionRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverPartitionsRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverServicePartitionsRequestMessageBody.h"
#include "Reliability/Failover/fm/RecoverSystemPartitionsRequestMessageBody.h"
#include "Reliability/Failover/fm/CreateServiceMessageBody.h"
#include "Reliability/Failover/fm/CreateServiceReplyMessageBody.h"
#include "Reliability/Failover/fm/UpdateServiceMessageBody.h"
#include "Reliability/Failover/fm/UpdateServiceReplyMessageBody.h"
#include "Reliability/Failover/fm/ServiceTableRequestMessageBody.h"
#include "Reliability/Failover/fm/ServiceTableUpdateMessageBody.h"
#include "Reliability/Failover/fm/ActivateNodeRequestMessageBody.h"
#include "Reliability/Failover/fm/UpdateSystemServiceMessageBody.h"
#include "Reliability/Failover/fm/DeleteApplicationRequestMessageBody.h"
#include "Reliability/Failover/fm/DeleteApplicationReplyMessageBody.h"
#include "Reliability/Failover/fm/DeleteServiceMessageBody.h"
#include "Reliability/Failover/fm/ServiceDescriptionRequestMessageBody.h"
#include "Reliability/Failover/fm/ServiceDescriptionReplyMessageBody.h"
#include "Reliability/Failover/fm/ResetPartitionLoadRequestMessageBody.h"
#include "Reliability/Failover/fm/ToggleVerboseServicePlacementHealthReportingRequestMessageBody.h"
#include "Reliability/Failover/fm/CreateApplicationRequestMessageBody.h"
#include "Reliability/Failover/fm/UpdateApplicationRequestMessageBody.h"

#include "Reliability/Failover/fm/ServiceLookupTable.h"
#include "Reliability/Failover/fm/ChangeNotificationMessageBody.h"

#include "Reliability/Failover/fm/ServiceFactory.h"
#include "Reliability/Failover/fm/IFailoverManager.h"


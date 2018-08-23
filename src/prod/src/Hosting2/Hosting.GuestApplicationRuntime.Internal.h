// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

//
// Header Files required by internal Runtime header files
//

#include "Hosting2/Hosting.Internal.h"
#include "Hosting2/Hosting.Runtime.Internal.h"

#include "Hosting2/IGuestServiceTypeHost.h"
#include "Hosting2/IGuestServiceInstance.h"
#include "Hosting2/IGuestServiceReplica.h"

#include "Hosting2/GuestServiceTypeHost.h"
#include "Hosting2/InProcessApplicationHost.h"
#include "Hosting2/InProcessApplicationHostCodePackageActivator.h"
#include "Hosting2/ComGuestServiceCodePackageActivationManager.h"

#include "Hosting2/ComDummyReplicator.h"
#include "Hosting2/ComGuestServiceInstance.h"
#include "Hosting2/ComGuestServiceReplica.h"
#include "Hosting2/ComStatelessGuestServiceTypeFactory.h"
#include "Hosting2/ComStatefulGuestServiceTypeFactory.h"

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace TracePoints
{
    namespace WinFabricStructuredTracingCodes
    {
        enum Enum
        {
            FM_NodeAddedToGFUM = 18448,
            FMM_NodeAddedToGFUM = 19472,
            AuthorityOwnerInnerCreateNameReplyReceiveComplete = 22022,
            NameOwnerInnerCreateNameRequestReceiveComplete = 22023,
            NameOwnerInnerCreateNameRequestProcessComplete = 22024,
            AuthorityOwnerInnerDeleteNameReplyReceiveComplete = 22026,
            NameOwnerInnerDeleteNameRequestReceiveComplete = 22027,
            NameOwnerInnerDeleteNameRequestProcessComplete = 22028,
            AuthorityOwnerInnerCreateServiceReplyFromNameOwnerReceiveComplete = 22030,
            NameOwnerInnerCreateServiceReplyFromFMReceiveComplete = 22032,
            NameOwnerInnerCreateServiceRequestReceiveComplete = 22033,
            NameOwnerInnerCreateServiceRequestProcessComplete = 22034,
            AuthorityOwnerInnerDeleteServiceReplyFromNameOwnerReceiveComplete = 22036,
            NameOwnerInnerDeleteServiceReplyFromFMReceiveComplete = 22038,
            NameOwnerInnerDeleteServiceRequestReceiveComplete = 22039,
            NameOwnerInnerDeleteServiceRequestProcessComplete = 22040,
            StartupRepairCompleted = 22045,
            AuthorityOwnerHierarchyRepairCompleted = 22046,
            AuthorityOwnerServiceRepairCompleted = 22047
        };
    };
};

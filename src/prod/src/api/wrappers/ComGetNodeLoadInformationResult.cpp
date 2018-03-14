// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

namespace Api
{
    ComGetNodeLoadInformationResult::ComGetNodeLoadInformationResult(
        NodeLoadInformationQueryResult && NodeLoadInformation)
        : NodeLoadInformation_()
        , heap_()
    {
        NodeLoadInformation_ = heap_.AddItem<FABRIC_NODE_LOAD_INFORMATION>();
        NodeLoadInformation.ToPublicApi(heap_, *NodeLoadInformation_.GetRawPointer());
    }

    const FABRIC_NODE_LOAD_INFORMATION *STDMETHODCALLTYPE ComGetNodeLoadInformationResult::get_NodeLoadInformation (void)
    {
        return NodeLoadInformation_.GetRawPointer();
    }
}

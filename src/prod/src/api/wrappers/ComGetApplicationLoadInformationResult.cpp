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
    ComGetApplicationLoadInformationResult::ComGetApplicationLoadInformationResult(
        ApplicationLoadInformationQueryResult && applicationLoadInformation)
        : applicationLoadInformation_()
        , heap_()
    {
        applicationLoadInformation_ = heap_.AddItem<FABRIC_APPLICATION_LOAD_INFORMATION>();
        applicationLoadInformation.ToPublicApi(heap_, *applicationLoadInformation_.GetRawPointer());
    }

    const FABRIC_APPLICATION_LOAD_INFORMATION *STDMETHODCALLTYPE ComGetApplicationLoadInformationResult::get_ApplicationLoadInformation (void)
    {
        return applicationLoadInformation_.GetRawPointer();
    }
}

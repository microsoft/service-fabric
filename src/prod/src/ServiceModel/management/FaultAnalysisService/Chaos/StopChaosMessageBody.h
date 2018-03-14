// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace FaultAnalysisService
    {
        class StopChaosMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            StopChaosMessageBody() { }
        };
    }
}

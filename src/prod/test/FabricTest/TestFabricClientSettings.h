// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class FabricTestSession;

    class TestFabricClientSettings
    {
        DENY_COPY(TestFabricClientSettings);

    public:
        TestFabricClientSettings();
        ~TestFabricClientSettings() {}

        bool SetSettings(Common::StringCollection const & params);
        
        void CreateFabricSettingsClient(__in FabricTestFederation & testFederation);

    private:
        bool ForceOpen();

        Common::ComPointer<Api::ComFabricClient> comFabricClient_;
        Common::ComPointer<IFabricClientSettings2> client_;
        Common::ComPointer<IFabricQueryClient> queryClient_;
    };
}

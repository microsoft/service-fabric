// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ModifyUpgradeHelper
        {
        public:

            // use template instead of base class for version compatibility
            // (fields can only be extended now in derived classes)
            //
            template <class TUpgradeDescription, class TUpdateDescription>
            static bool TryModifyUpgrade(
                __in TUpgradeDescription & upgrade,
                TUpdateDescription const & update,
                __out std::wstring & validationErrorMessage);

            template <
                class TInternalDescription, 
                class TPublicDescription, 
                class TInternalHealthPolicy, 
                class TPublicHealthPolicy>
            static Common::ErrorCode FromPublicUpdateDescription(
                __in TInternalDescription & internalDescription,
                TPublicDescription const & publicDescription);
        };
    }
}

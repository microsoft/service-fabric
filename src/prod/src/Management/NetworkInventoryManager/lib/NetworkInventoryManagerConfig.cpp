// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Management::NetworkInventoryManager;

DEFINE_SINGLETON_COMPONENT_CONFIG(Management::NetworkInventoryManager::NetworkInventoryManagerConfig)

bool NetworkInventoryManagerConfig::IsNetworkInventoryManagerEnabled()
{
    return GetConfig().IsEnabled && CommonConfig::GetConfig().EnableUnsupportedPreviewFeatures;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include <memory>

namespace KtlLogger
{
    class KtlLoggerNode;
    typedef std::shared_ptr<KtlLoggerNode> KtlLoggerNodeSPtr;
    typedef std::unique_ptr<KtlLoggerNode> KtlLoggerNodeUPtr;

    class SharedLogSettings;
    typedef std::shared_ptr<SharedLogSettings> SharedLogSettingsSPtr;
    typedef std::unique_ptr<SharedLogSettings> SharedLogSettingsUPtr;
}

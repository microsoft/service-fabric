// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(DiagnosticsSinkBase)

DiagnosticsSinkBase * DiagnosticsSinkBase::CreateNew(DiagnosticsSinkKind::Enum kind)
{
    switch(kind)
    {
    case DiagnosticsSinkKind::Enum::AzureInternalMonitoringPipeline:
        //return DiagnosticsSinkTypeActivator<DiagnosticsSinkKind::Enum::AzureInternalMonitoringPipeline>::CreateNew();
        return new DiagnosticsSinkBase(L"AzureInternalMonitoringPipeline");
    default:
        //return new DiagnosticsSinkBase(DiagnosticsSinkKind::Enum::Invalid);
        return new DiagnosticsSinkBase(L"Invalid");
    }
}

DiagnosticsSinkBaseSPtr DiagnosticsSinkBase::CreateSPtr(DiagnosticsSinkKind::Enum kind)
{
    switch(kind)
    {
    case DiagnosticsSinkKind::Enum::AzureInternalMonitoringPipeline:
//        return DiagnosticsSinkTypeActivator<DiagnosticsSinkKind::Enum::AzureInternalMonitoringPipeline>::CreateSPtr();
        return make_shared<DiagnosticsSinkBase>(L"AzureInternalMonitoringPipeline");
    default:
//        return make_shared<DiagnosticsSinkBase>(DiagnosticsSinkKind::Enum::Invalid);
        return make_shared<DiagnosticsSinkBase>(L"Invalid");
    }
}
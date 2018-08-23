//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(InstanceState)
INITIALIZE_SIZE_ESTIMATION(CodePackageInstanceView)

InstanceState::InstanceState(
    wstring const & state,
    DWORD const exitCode)
    : state_(state)
    , exitCode_(exitCode)
{
}

CodePackageInstanceView::CodePackageInstanceView(
    int restartCount,
    InstanceState const & currentState,
    InstanceState const & previousState)
    : restartCount_(restartCount)
    , currentState_(currentState)
    , previousState_(previousState)
{
}

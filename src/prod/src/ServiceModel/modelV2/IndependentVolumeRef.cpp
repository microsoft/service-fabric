//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel::ModelV2;

INITIALIZE_SIZE_ESTIMATION(IndependentVolumeRef)

bool IndependentVolumeRef::operator==(IndependentVolumeRef const & other) const
{
    return ContainerVolumeRef::operator==(other);
}

bool IndependentVolumeRef::operator!=(IndependentVolumeRef const & other) const
{
    return !(*this == other);
}

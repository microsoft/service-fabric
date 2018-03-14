// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;

Index::Index(int partitionIndex, int itemIndex) :
    partitionIndex_(partitionIndex),
    itemIndex_(itemIndex)
{
}

Index::~Index()
{}

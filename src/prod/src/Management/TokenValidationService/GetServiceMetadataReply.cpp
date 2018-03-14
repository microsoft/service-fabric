// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::TokenValidationService;
using namespace ServiceModel;

GetServiceMetadataReply::GetServiceMetadataReply() 
    : metadata_()  
    , error_()
{
}

GetServiceMetadataReply::GetServiceMetadataReply(
    TokenValidationServiceMetadata const & metadata,
    ErrorCode const & error)
    : metadata_(metadata)
    , error_(error)
{
}

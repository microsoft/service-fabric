// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_OPERATION_TYPE const & val);
Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_OPERATION_METADATA const & val);
Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_SERVICE_PARTITION_ACCESS_STATUS const & val);
Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_REPLICA_SET_QUORUM_MODE const & val);
Common::TextWriter& operator << (__in Common::TextWriter & w, FABRIC_EPOCH const & val);

bool operator == (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b);
bool operator != (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b);
bool operator < (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b);
bool operator <= (FABRIC_EPOCH const & a, FABRIC_EPOCH const & b);

HRESULT ValidateOperation(
    Common::ComPointer<IFabricOperationData> const & operationPointer, 
    LONG maxSize,
    __out_opt std::wstring & errorMessage);

HRESULT ValidateBatchOperation(
    Common::ComPointer<IFabricBatchOperationData> const & batchOperationPointer, 
    LONG maxSize,
    __out_opt std::wstring & errorMessage);

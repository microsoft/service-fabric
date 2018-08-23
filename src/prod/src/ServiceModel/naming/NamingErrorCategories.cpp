// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace std;

    //
    // ErrorCodes listed in IsRetryableAtGateway() and ToClientError() should not overlap
    //
    bool NamingErrorCategories::IsRetryableAtGateway(ErrorCode const & error)
    {
        switch (error.ReadValue())
        {
        // Naming Store Service stabilization
        case ErrorCodeValue::FMFailoverUnitNotFound:
        case ErrorCodeValue::SystemServiceNotFound:
        case ErrorCodeValue::MessageHandlerDoesNotExistFault:
        case ErrorCodeValue::RoutingNodeDoesNotMatchFault:
        case ErrorCodeValue::NodeIsNotRoutingFault:

        // Clock drift
        case ErrorCodeValue::StaleRequest:

        // FM stabilization
        case ErrorCodeValue::FMLocationNotKnown:
        case ErrorCodeValue::FMNotReadyForUse:
        case ErrorCodeValue::FMStoreNotUsable:
        case ErrorCodeValue::FMStoreUpdateFailed:
        case ErrorCodeValue::UpdatePending:

        // FileStoreService operation
        case ErrorCodeValue::FileStoreServiceNotReady:
        case ErrorCodeValue::FileUpdateInProgress:

        // RA Query
        case ErrorCodeValue::RANotReadyForUse:

            return true;

        default:
            return false;
        }
    }

    ErrorCode NamingErrorCategories::ToClientError(ErrorCode && error)
    {
        switch (error.ReadValue())
        {
        // Do not automatically retry on these errors at gateway since the
        // system service may have already done some processing
        // and we may return an unexpected error code when
        // retrying. Convert these to OperationCanceled so the
        // client can retry.
        //
        // This only applies to write operations but we currently
        // do not distinguish write/read operations.
        // 
        case ErrorCodeValue::REQueueFull:
        case ErrorCodeValue::NotPrimary:
        case ErrorCodeValue::ReconfigurationPending:
        case ErrorCodeValue::NoWriteQuorum:
        case ErrorCodeValue::StoreOperationCanceled:
        case ErrorCodeValue::StoreUnexpectedError:
        case ErrorCodeValue::StoreTransactionNotActive:
        case ErrorCodeValue::TransactionAborted:

        // Store service couldn't queue the operation in job queue or is otherwise overloaded.
        // This can happen for both read and write operations.
        case ErrorCodeValue::NamingServiceTooBusy:

            return ErrorCode(ErrorCodeValue::OperationCanceled, error.TakeMessage());

        default:
            return move(error);
        }
    }

    bool NamingErrorCategories::ShouldRevertCreateService(ErrorCode const & error)
    {
        switch(error.ReadValue())
        {
        // Triggers workflow on both the Name Owner and Authority Owner
        // to revert tentative create service state and fail the
        // request with the error instead of retrying.
        //
        // PLB errors
        //
        case ErrorCodeValue::ServiceAffinityChainNotSupported:
        case ErrorCodeValue::InsufficientClusterCapacity:
        case ErrorCodeValue::StaleServicePackageInstance:
        case ErrorCodeValue::ConstraintKeyUndefined:
        case ErrorCodeValue::InvalidServiceScalingPolicy:
        //
        // Generic unexpected errors (avoid getting stuck)
        //
        case ErrorCodeValue::FMServiceDoesNotExist:
        case ErrorCodeValue::InvalidNameUri:
        case ErrorCodeValue::InvalidArgument:
        case ErrorCodeValue::InvalidOperation:
        case ErrorCodeValue::InvalidMessage:
        case ErrorCodeValue::OperationFailed:
            return true;

        default:
            return false;
        }
    }
}

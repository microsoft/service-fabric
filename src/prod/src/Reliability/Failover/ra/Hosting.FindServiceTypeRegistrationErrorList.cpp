// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Hosting;

FindServiceTypeRegistrationErrorList::FindServiceTypeRegistrationErrorList(
    std::map<ErrorCodeValue::Enum, ErrorType::Enum> && values,
    bool isUnknownErrorFatal)
    : values_(move(values)),
    isUnknownErrorFatal_(move(isUnknownErrorFatal))
{
}

FindServiceTypeRegistrationErrorList::FindServiceTypeRegistrationErrorList(FindServiceTypeRegistrationErrorList && other)
    : values_(move(other.values_)),
    isUnknownErrorFatal_(move(other.isUnknownErrorFatal_))
{
}

FindServiceTypeRegistrationErrorCode FindServiceTypeRegistrationErrorList::TranslateError(Common::ErrorCode && error) const
{
    auto isRetryable = IsRetryableError(error.ReadValue());
    return FindServiceTypeRegistrationErrorCode(move(error), isRetryable);
}

bool FindServiceTypeRegistrationErrorList::IsRetryableError(ErrorCodeValue::Enum error) const
{
    auto iter = values_.find(error);
    if (iter == values_.cend())
    {
        return !isUnknownErrorFatal_;
    }

    return iter->second == ErrorType::Retryable;
}

FindServiceTypeRegistrationErrorList FindServiceTypeRegistrationErrorList::CreateErrorListForNewReplica()
{
    map<ErrorCodeValue::Enum, ErrorType::Enum> values;

    // During replica open the following errors are transient errors that are retryable
    // All other errors are fatal errors that should cause the replica to be aborted
    values[ErrorCodeValue::HostingDeploymentInProgress] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingActivationInProgress] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServiceTypeNotRegistered] = ErrorType::Retryable;
    values[ErrorCodeValue::NotFound] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingApplicationVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServicePackageVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingFabricRuntimeNotRegistered] = ErrorType::Retryable;

    values[ErrorCodeValue::HostingServiceTypeDisabled] = ErrorType::Fatal;
    values[ErrorCodeValue::ApplicationInstanceDeleted] = ErrorType::Fatal;

    return FindServiceTypeRegistrationErrorList(move(values), true /* isUnknownErrorFatal */);
}

FindServiceTypeRegistrationErrorList FindServiceTypeRegistrationErrorList::CreateErrorListForExistingReplica()
{
    map<ErrorCodeValue::Enum, ErrorType::Enum> values;

    // During replica reopen all errors are retryable (including ServiceTypeDisabled)
    // This is because persisted replicas should not be dropped by the RA unless it is explicit
    // The below list is kept so that the contract is clear and explicit and in future 
    // if this code is refactored the errors here are considered

    values[ErrorCodeValue::HostingDeploymentInProgress] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingActivationInProgress] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServiceTypeNotRegistered] = ErrorType::Retryable;
    values[ErrorCodeValue::NotFound] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingApplicationVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServicePackageVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingServiceTypeRegistrationVersionMismatch] = ErrorType::Retryable;
    values[ErrorCodeValue::HostingFabricRuntimeNotRegistered] = ErrorType::Retryable;

    values[ErrorCodeValue::HostingServiceTypeDisabled] = ErrorType::Retryable;

    // The error below is fatal because the application has been deleted
    // and unprovisioned from image store so the replica can never be opened again
    // The RA must drop this replica without calling CR(None) and assume some external process will clean up the state
    values[ErrorCodeValue::ApplicationInstanceDeleted] = ErrorType::Fatal;

    return FindServiceTypeRegistrationErrorList(move(values), false /* isUnknownErrorFatal */);
}

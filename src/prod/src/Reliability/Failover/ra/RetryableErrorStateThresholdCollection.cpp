// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

RetryableErrorStateThresholdCollection::RetryableErrorStateThresholdCollection()
{
}

RetryableErrorStateThresholdCollection RetryableErrorStateThresholdCollection::CreateDefault()
{
    RetryableErrorStateThresholdCollection rv;
    rv.AddThreshold(
        RetryableErrorStateName::FindServiceRegistrationAtOpen,
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::FindServiceRegistrationAtReopen,
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::FindServiceRegistrationAtDrop,
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationWarningReportThresholdAtDrop; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationMaxRetryThresholdAtDrop; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ServiceTypeRegistrationErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::ReplicaOpen,
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::ReplicaReopen,
        [](FailoverConfig const & cfg) { return cfg.ReplicaReopenFailureWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaReopenFailureMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaOpenFailureErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::ReplicaChangeRoleAtCatchup,
        [](FailoverConfig const & cfg) { return cfg.ReplicaChangeRoleFailureWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaChangeRoleFailureMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaChangeRoleFailureRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaChangeRoleFailureErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::ReplicaClose,
        [](FailoverConfig const & cfg) { return cfg.ReplicaCloseFailureWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaCloseFailureMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaCloseFailureRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaCloseFailureErrorReportThreshold; });

    rv.AddThreshold(
        RetryableErrorStateName::ReplicaDelete,
        [](FailoverConfig const & cfg) { return cfg.ReplicaDeleteFailureWarningReportThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaDeleteFailureMaxRetryThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaDeleteFailureRestartThreshold; },
        [](FailoverConfig const & cfg) { return cfg.ReplicaDeleteFailureErrorReportThreshold; });
    
    rv.AddThreshold(RetryableErrorStateName::None, INT_MAX, INT_MAX, INT_MAX, INT_MAX);

    return rv;
}

RetryableErrorStateThresholdCollection::Threshold RetryableErrorStateThresholdCollection::GetThreshold(
    RetryableErrorStateName::Enum state,
    FailoverConfig const & config) const
{
    auto it = thresholds_.find(state);
    ASSERT_IF(it == thresholds_.cend(), "Threshold must be defined {0}", static_cast<int>(state));

    Threshold rv;
    rv.WarningThreshold = it->second.WarningThresholdReader(config);
    rv.DropThreshold = it->second.DropThresholdReader(config);
    rv.RestartThreshold = it->second.RestartThresholdReader(config);
    rv.ErrorThreshold = it->second.ErrorThresholdReader(config);
    return rv;
}

void RetryableErrorStateThresholdCollection::AddThreshold(
    RetryableErrorStateName::Enum state,
    int warningThreshold,
    int dropThreshold,
    int restartThreshold,
    int errorThreshold)
{
    Storage s;
    s.WarningThresholdReader = [warningThreshold](FailoverConfig const &) { return warningThreshold; };
    s.DropThresholdReader = [dropThreshold](FailoverConfig const &) { return dropThreshold; };
    s.RestartThresholdReader = [restartThreshold](FailoverConfig const &) { return restartThreshold; };
    s.ErrorThresholdReader = [errorThreshold](FailoverConfig const &) { return errorThreshold; };
    
    Insert(state, s);
}

void RetryableErrorStateThresholdCollection::AddThreshold(
    RetryableErrorStateName::Enum state,
    DynamicThresholdReadFunctionPtr warningThresholdReader,
    DynamicThresholdReadFunctionPtr dropThresholdReader,
    DynamicThresholdReadFunctionPtr restartThresholdReader,
    DynamicThresholdReadFunctionPtr errorThresholdReader)
{
    Storage s;
    s.WarningThresholdReader = warningThresholdReader;
    s.DropThresholdReader = dropThresholdReader;
    s.RestartThresholdReader = restartThresholdReader;
    s.ErrorThresholdReader = errorThresholdReader;
    
    Insert(state, s);
}

void RetryableErrorStateThresholdCollection::Insert(RetryableErrorStateName::Enum state, Storage s)
{
    thresholds_[state] = s;
}

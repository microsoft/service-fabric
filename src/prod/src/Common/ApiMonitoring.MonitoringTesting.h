// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
		namespace MonitoringTesting
		{
			namespace EnabledTraceFlags
			{
				enum Enum
				{
					None = 0,
					Slow = 1,
					LifeCycle = 2,
					All = 3
				};
			}

			class CallDescriptionTesting
			{
			public:
				static ApiCallDescriptionSPtr Create(
					int64 replicaId)
				{
					return Create(replicaId, TimeSpan::MaxValue, Stopwatch::Now());
				}

				static ApiCallDescriptionSPtr Create(
					int64 replicaId,
					StopwatchTime startTime)
				{
					return Create(replicaId, TimeSpan::MaxValue, startTime);
				}

				static ApiCallDescriptionSPtr Create(
					int64 replicaId,
					TimeSpan slowDuration,
					StopwatchTime startTime)
				{
					return Create(replicaId, true, EnabledTraceFlags::All, slowDuration, startTime);
				}

				static ApiCallDescriptionSPtr Create(
					int64 replicaId,
					bool enableHealth,
					EnabledTraceFlags::Enum traceFlags,
					TimeSpan slowDuration)
				{
					return Create(replicaId, enableHealth, traceFlags, slowDuration, Stopwatch::Now());
				}

				static ApiCallDescriptionSPtr Create(
					int64 replicaId,
					bool enableHealth,
					EnabledTraceFlags::Enum traceFlags,
					TimeSpan slowDuration,
					StopwatchTime startTime)
				{
					return Create(replicaId, enableHealth, traceFlags, slowDuration, startTime, TimeSpan::MaxValue);
				}

				static ApiCallDescriptionSPtr Create(
					int64 replicaId,
					bool enableHealth,
					EnabledTraceFlags::Enum traceFlags,
					TimeSpan slowDuration,
					StopwatchTime startTime,
					TimeSpan periodicTraceDuration)
				{
					bool slowTraceEnabled = traceFlags == EnabledTraceFlags::Slow || traceFlags == EnabledTraceFlags::All;
					bool lifeCycleTraceEnabled = traceFlags == EnabledTraceFlags::LifeCycle || traceFlags == EnabledTraceFlags::All;

					ApiNameDescription nameDesc(InterfaceName::LastValidEnum, ApiName::LastValidEnum, L"");
					auto data = MonitoringData(Guid(), replicaId, 0, std::move(nameDesc), startTime);
					MonitoringParameters parameters(enableHealth, slowTraceEnabled, lifeCycleTraceEnabled, slowDuration, periodicTraceDuration);
					return std::make_shared<ApiCallDescription>(std::move(data), parameters);
				}
			};
        }
    }
}



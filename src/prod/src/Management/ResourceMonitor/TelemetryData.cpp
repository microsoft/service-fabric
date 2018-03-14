// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

Common::WStringLiteral const ResourceMonitor::Metric::NameParameter(L"name");
Common::WStringLiteral const ResourceMonitor::Metric::ValueParameter(L"value");

Common::WStringLiteral const ResourceMonitor::Properties::PartitionParameter(L"partition");

Common::WStringLiteral const ResourceMonitor::BaseData::MetricParameter(L"metrics");
Common::WStringLiteral const ResourceMonitor::BaseData::PropertyParameter(L"properties");

Common::WStringLiteral const ResourceMonitor::Data::BaseTypeParameter(L"baseType");
Common::WStringLiteral const ResourceMonitor::Data::BaseDataParameter(L"baseData");

Common::WStringLiteral const ResourceMonitor::Envelope::IKeyParameter(L"iKey");
Common::WStringLiteral const ResourceMonitor::Envelope::TimeParameter(L"time");
Common::WStringLiteral const ResourceMonitor::Envelope::NameParameter(L"name");
Common::WStringLiteral const ResourceMonitor::Envelope::DataParameter(L"data");


Envelope Envelope::CreateTelemetryMessage(TelemetryEvent && event, std::wstring const & instrumentationKey)
{
    Envelope envelope;

    envelope.Name = L"Resources";
    envelope.Ikey = instrumentationKey;
    envelope.Time = move(Common::Stopwatch::Now().ToDateTime().ToString());
    envelope.DataValue.BaseType = L"MetricData";
    envelope.DataValue.BaseDataValue.Property.PartitionId = move(event.PartitionId);
    envelope.DataValue.BaseDataValue.Metrics = move(event.Metrics);

    return envelope;
}

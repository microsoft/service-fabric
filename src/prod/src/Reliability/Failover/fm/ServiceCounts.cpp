// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ServiceCounts::ServiceCounts()
    : serviceCount_(0),
      statelessCount_(0),
      volatileCount_(0),
      persistedCount_(0),
      updatingCount_(0),
      deletingCount_(0),
      deletedCount_(0),
      serviceTypeCount_(0),
      applicationCount_(0),
      applicationUpgradeCount_(0),
      monitoredUpgradeCount_(0),
      manualUpgradeCount_(0),
      forceRestartUpgradeCount_(0),
      notificationOnlyUpgradeCount_(0)
{
}

string ServiceCounts::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "Services={0}\r\n  Stateless={1}\r\n  Volatile={2}\r\n  Persisted={3}\r\nUpdating={4}\r\nDeleting={5}\r\nDeleted={6}\r\nServiceTypes={7}\r\nApplications={8}\r\nApplicationUpgrades={9}\r\n  Monitored={10}\r\n  Manual={11}\r\n  ForceRestart={12}\r\n  NotificationOnly={13}";
    size_t index = 0;

    traceEvent.AddEventField<size_t>(format, name + ".serviceCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".statelessCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".volatileCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".persistedCount", index);

    traceEvent.AddEventField<size_t>(format, name + ".updatingCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".deletingCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".deletedCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".serviceTypeCount", index);

    traceEvent.AddEventField<size_t>(format, name + ".applicationCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".applicationUpgradeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".monitoredUpgradeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".manualUpgradeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".forceRestartUpgradeCount", index);
    traceEvent.AddEventField<size_t>(format, name + ".notificationOnlyUpgradeCount", index);

    return format;
}

void ServiceCounts::FillEventData(Common::TraceEventContext & context) const
{
    context.Write(serviceCount_);
    context.Write(statelessCount_);
    context.Write(volatileCount_);
    context.Write(persistedCount_);

    context.Write(updatingCount_);
    context.Write(deletingCount_);
    context.Write(deletedCount_);
    context.Write(serviceTypeCount_);

    context.Write(applicationCount_);
    context.Write(applicationUpgradeCount_);
    context.Write(monitoredUpgradeCount_);
    context.Write(manualUpgradeCount_);
    context.Write(forceRestartUpgradeCount_);
    context.Write(notificationOnlyUpgradeCount_);
}

void ServiceCounts::WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const
{
    writer.WriteLine("Services={0} (StatelessServices={1}, VolatileServices={2}, PersistedServices={3})",
      serviceCount_,
      statelessCount_,
      volatileCount_,
      persistedCount_);

    writer.WriteLine("UpdatingServices={0}, DeletingServices={1}, DeletedServices={2}, ServiceTypes={3}",
      updatingCount_,
      deletingCount_,
      deletedCount_,
      serviceTypeCount_);

    writer.WriteLine("Applications={0}, ApplicationUpgrades={1} (MonitoredUpgrades={2}, ManualUpgrades={3}, ForceRestartUpgrades={4}, NotificationOnlyUpgrades={5})",
      applicationCount_,
      applicationUpgradeCount_,
      monitoredUpgradeCount_,
      manualUpgradeCount_,
      forceRestartUpgradeCount_,
      notificationOnlyUpgradeCount_);
}

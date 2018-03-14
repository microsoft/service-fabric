// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;

Common::WStringLiteral const ContainerEventConfig::StatusParameter(L"status");
Common::WStringLiteral const ContainerEventConfig::IdParameter(L"id");
Common::WStringLiteral const ContainerEventConfig::FromParameter(L"from");
Common::WStringLiteral const ContainerEventConfig::TypeParameter(L"Type");
Common::WStringLiteral const ContainerEventConfig::ActionParameter(L"Action");
Common::WStringLiteral const ContainerEventConfig::ActorConfigParameter(L"Actor");
Common::WStringLiteral const ContainerEventConfig::TimeParameter(L"time");
Common::WStringLiteral const ContainerEventConfig::TimeNanoParameter(L"timeNano");

Common::WStringLiteral const ContainerActorConfig::IdParameter(L"ID");
Common::WStringLiteral const ContainerActorConfig::AttributesParameter(L"Attributes");

Common::WStringLiteral const ContainerEventAttribute::NameParameter(L"name");
Common::WStringLiteral const ContainerEventAttribute::ExitCodeParameter(L"exitCode");

wstring const ContainerEventConfig::HealthEventString(L"health_status");
wstring const ContainerEventConfig::HealthyHealthStatusString(L"health_status: healthy");
wstring const ContainerEventConfig::UnhealthyHealthStatusString(L"health_status: unhealthy");

ContainerEventConfig::ContainerEventConfig() :
    Status(),
    Id(),
    From(),
    Type(),
    Action(),
    ActorConfig(),
    Time(),
    TimeNano()
{
}

bool ContainerEventConfig::IsHealthEvent() const
{
    return StringUtility::StartsWith(this->Action, HealthEventString);
}

bool ContainerEventConfig::IsHealthStatusHealthy() const
{
    return (StringUtility::Compare(this->Action, HealthyHealthStatusString) == 0);
}

ContainerEventConfig::~ContainerEventConfig()
{
}

ContainerActorConfig::ContainerActorConfig() : Id(), Attributes()
{}

ContainerActorConfig::~ContainerActorConfig()
{
}

ContainerEventAttribute::ContainerEventAttribute() : Name(), ExitCode()
{
}

ContainerEventAttribute::~ContainerEventAttribute()
{
}

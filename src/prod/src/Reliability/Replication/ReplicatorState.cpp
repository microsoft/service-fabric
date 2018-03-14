// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::ErrorCode;
using Common::StringWriter;
using Common::wformatString;

using std::wstring;

ReplicatorState::ReplicatorState()
    :   state_(Created),
        role_(::FABRIC_REPLICA_ROLE_NONE)
{
}

ReplicatorState::~ReplicatorState()
{
}

ErrorCode ReplicatorState::TransitionToOpened()
{
    if (state_ != Created)
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }

    state_ = Opened;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToChangingRole(
    ::FABRIC_REPLICA_ROLE newRole,
    __out Action & nextAction)
{
    nextAction = None;
        
    switch (state_)
    {
    case Opened:
        switch (newRole)
        {
        case ::FABRIC_REPLICA_ROLE_PRIMARY:
            nextAction = CreateInitialPrimary;
            break;
        case ::FABRIC_REPLICA_ROLE_IDLE_SECONDARY:
        case ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            nextAction = CreateInitialSecondary;
            break;
        case ::FABRIC_REPLICA_ROLE_NONE:
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
        break;
    case Primary:
        switch (newRole)
        {
        case ::FABRIC_REPLICA_ROLE_PRIMARY:
            Assert::CodingError("ChangeRole called on primary with no role change, should call UpdateEpoch instead");
            break;
        case ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            nextAction = DemotePrimaryToSecondary;
            break;
        case ::FABRIC_REPLICA_ROLE_NONE:
            nextAction = ClosePrimary;
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
        break;
    case SecondaryActive:
        // Active can only transition to primary
        switch (newRole)
        {
        case ::FABRIC_REPLICA_ROLE_PRIMARY:
            nextAction = PromoteSecondaryToPrimary;
            break;
        case ::FABRIC_REPLICA_ROLE_NONE:
            nextAction = CloseSecondary;
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
        break;
    case SecondaryIdle:
        switch (newRole)
        {
        case ::FABRIC_REPLICA_ROLE_PRIMARY:
            nextAction = PromoteSecondaryToPrimary;
            break;
        case ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY:
            nextAction = PromoteIdleToActive;
            break;
        case ::FABRIC_REPLICA_ROLE_NONE:
            nextAction = CloseSecondary;
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
        break;
    case Faulted:
        switch (newRole)
        {
        case ::FABRIC_REPLICA_ROLE_NONE:
            nextAction = None;
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
        break;
    default:
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }

    // If we got to this point, then the transition is valid
    state_ = ChangingRole;
    role_ = newRole;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToPrimary()
{
    ASSERT_IFNOT(
        state_ == ChangingRole || state_ == CheckingDataLoss, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        Primary);

    state_ = Primary;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToRoleNone()
{
    ASSERT_IFNOT(
        state_ == ChangingRole, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        RoleNone);

    state_ = RoleNone;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToSecondaryActive()
{
    ASSERT_IFNOT(
        state_ == ChangingRole, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        SecondaryActive);

    state_ = SecondaryActive;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToSecondaryIdle()
{
    ASSERT_IFNOT(
        state_ == ChangingRole, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        SecondaryIdle);

    state_ = SecondaryIdle;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToCheckingDataLoss()
{
    if (state_ != Primary)
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }

    state_ = CheckingDataLoss;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToClosing()
{
    if (state_ != Opened && 
        state_ != Primary && 
        state_ != SecondaryActive &&
        state_ != SecondaryIdle && 
        state_ != Faulted && 
        state_ != None && 
        state_ != RoleNone)
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }

    state_ = Closing;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToClosed()
{
    ASSERT_IFNOT(
        state_ == Closing, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        Closed);

    state_ = Closed;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToAborting()
{
    if (state_ != Created && 
        state_ != Opened && 
        state_ != Primary && 
        state_ != SecondaryActive &&
        state_ != SecondaryIdle && 
        state_ != Faulted && 
        state_ != Closing &&
        state_ != Closed && 
        state_ != None && 
        state_ != RoleNone)
    {
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }

    state_ = Aborting;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode ReplicatorState::TransitionToAborted()
{
    ASSERT_IFNOT(
        state_ == Aborting, 
        "Invalid state transition. Current {0}, Desired {1}",
        ToString(),
        Aborting);

    state_ = Aborted;
    return ErrorCode(Common::ErrorCodeValue::Success);
}

void ReplicatorState::TransitionToFaulted()
{
    state_ = Faulted;
}
    
wstring ReplicatorState::ToString() const
{
    wstring content;
    StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return content;
}

std::string ReplicatorState::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0}, role={1}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".state", index);
    traceEvent.AddEventField<wstring>(format, name + ".role", index);

    return format;
}

void ReplicatorState::FillEventData(Common::TraceEventContext & context) const
{
    wstring state;
    wstring role;

    switch (state_)
    {
        case Created: 
             state = L"Created";
             role = wformatString(role_);
             break;
        case Opened: 
             state = L"Opened";
             role = wformatString(role_);
            break;
        case ChangingRole:
             state = L"ChangingRole";
             role = wformatString(role_);
            break;
        case Primary:
             state = L"Primary";
             role = wformatString(role_);
            break;
        case SecondaryIdle:
        case SecondaryActive:
             state = L"Secondary";
             role = wformatString(role_);
            break;
        case CheckingDataLoss:
             state = L"CheckingDataLoss";
             role = wformatString(role_);
            break;
        case Closing:
             state = L"Closing";
             role = wformatString(role_);
            break;
        case Closed: 
             state = L"Closed";
             role = wformatString(role_);
            break;
        case Aborting:
             state = L"Aborting";
             role = wformatString(role_);
            break;
        case Aborted: 
             state = L"Aborted";
             role = wformatString(role_);
            break;
        case Faulted: 
             state = L"Faulted";
             role = wformatString(role_);
            break;
        case RoleNone: 
             state = L"RoleNone";
             role = wformatString(role_);
            break;
        default: 
            Assert::CodingError("Unknown replicator state");
     }

    context.WriteCopy<wstring>(state);
    context.WriteCopy<wstring>(role);
}

void ReplicatorState::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    switch (state_)
    {
        case Created: 
             w << "Created";
             break;
        case Opened: 
            w << "Opened";
            break;
        case ChangingRole:
            w << "ChangingRole" << role_;
            break;
        case Primary:
            w << "Primary";
            break;
        case SecondaryIdle:
        case SecondaryActive:
            w << "Secondary";
            break;
        case CheckingDataLoss:
            w << "CheckingDataLoss";
            break;
        case Closing:
            w << "Closing" << role_;
            break;
        case Closed: 
            w << "Closed" << role_;
            break;
        case Aborting:
            w << "Aborting" << role_;
            break;
        case Aborted: 
            w << "Aborted" << role_;
            break;
        case Faulted: 
            w << "Faulted" << role_;
            break;
        case RoleNone: 
            w << "RoleNone" << role_;
            break;
        default: 
            Assert::CodingError("Unknown replicator state");
    }    
}

} // end namespace ReplicationComponent
} // end namespace Reliability

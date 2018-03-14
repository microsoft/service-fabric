// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Infrastructure;

namespace
{
    class ValidationContext
    {
        DENY_COPY(ValidationContext);

    public:
        ValidationContext() {}

        __declspec(property(get = get_Tag)) std::string const & Tag;
        std::string const & get_Tag() const { return failureTag_; }

        void FailIf(bool condition, std::string const & tag)
        {
            if (condition)
            {
                Fail(tag);
            }
        }

        void Fail(std::string const & tag)
        {
            failureTag_ = tag;
            throw std::exception();
        }

    private:
        std::string failureTag_;
    };

    void ValidateServiceDescription(FailoverUnit const& old, FailoverUnit const& current, ValidationContext& context)
    {
        context.FailIf(old.ServiceDescription.Name != current.ServiceDescription.Name, "ServiceDescription");
        context.FailIf(old.ServiceDescription.Instance != current.ServiceDescription.Instance, "ServiceDescription::Instance");
        context.FailIf(old.ServiceDescription.UpdateVersion != current.ServiceDescription.UpdateVersion, "ServiceDescription::UpdateVersion");
    }

    void ValidateReplica(Replica const & old, Replica const & current, std::string const & prefix, ValidationContext& context)
    {
        context.FailIf(old.State != current.State, prefix + "state");
        context.FailIf(old.IntermediateConfigurationRole != current.IntermediateConfigurationRole, prefix + "icRole");
        
        context.FailIf(old.FederationNodeInstance != current.FederationNodeInstance, prefix + "nodeInstance");
        context.FailIf(old.ReplicaId != current.ReplicaId, prefix + "replicaId");
        context.FailIf(old.InstanceId != current.InstanceId, prefix + "instanceId");
        context.FailIf(old.CurrentConfigurationRole != current.CurrentConfigurationRole, prefix + "ccRole");
        context.FailIf(old.PreviousConfigurationRole != current.PreviousConfigurationRole, prefix + "pcRole");
        context.FailIf(old.IsUp != current.IsUp, prefix + "isUp");
        context.FailIf(old.State != current.State, prefix + "state");
    }

    void ValidateReplicas(FailoverUnit const & old, FailoverUnit const & current, ValidationContext& context)
    {
        context.FailIf(old.Replicas.size() != current.Replicas.size(), "Replica :: Size");

        for (size_t i = 0; i < old.Replicas.size(); ++i)
        {
            ValidateReplica(old.Replicas[i], current.Replicas[i], formatString("Replica {0}::", i), context);
        }
    }

    void ValidateFT(FailoverUnit const & old, FailoverUnit const & current, ValidationContext & context)
    {
        context.FailIf(old.State != current.State, "State");
        context.FailIf(old.LocalReplicaDeleted != current.LocalReplicaDeleted, "LocalReplicaDeleted");
        context.FailIf(old.IntermediateConfigurationEpoch != current.IntermediateConfigurationEpoch, "ICEpoch");
        context.FailIf(old.LocalReplicaId != current.LocalReplicaId, "ReplicaId");
        context.FailIf(old.LocalReplicaInstanceId != current.LocalReplicaInstanceId, "InstanceId");
        context.FailIf(old.DeactivationInfo != current.DeactivationInfo, "DeactivationInfo");

        context.FailIf(old.CurrentConfigurationEpoch != current.CurrentConfigurationEpoch, "CCEpoch");
        context.FailIf(old.PreviousConfigurationEpoch != current.PreviousConfigurationEpoch, "PCEpoch");

        ValidateServiceDescription(old, current, context);

        ValidateReplicas(old, current, context);
    };
}

void FailoverUnitPreCommitValidator::OnPreCommit(Infrastructure::EntityEntryBaseSPtr entry, Infrastructure::CommitDescription<FailoverUnit> const & description) 
{
    if (!Assert::IsTestAssertEnabled())
    {
        return;
    }

    // Currently only validate that if the commit is in memory it does not change any persisted state
    if (description.Type.StoreOperationType != Storage::Api::OperationType::Update || !description.Type.IsInMemoryOperation)
    {
        return;
    }

    FailoverUnitSPtr old;

    {
        auto readLock = entry->As<EntityEntry<FailoverUnit>>().CreateReadLock();
        old = make_shared<FailoverUnit>(*readLock.Current);
    }

    ValidationContext context;

    try
    {
        ValidateFT(*old, *description.Data, context);
    }
    catch (exception const &)
    {
        Assert::TestAssert("InMemoryCommit touched persisted state. Tag = {0}\r\n[New]{1}\r\n\r\n[Old]{2}", context.Tag, *old, *description.Data);
    }
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;


class LookupVersionGlobalStateFactory : public Federation::SerializableActivationFactory
{
public:
    static LookupVersionGlobalStateFactory* CreateSingleton()
    {
        LookupVersionGlobalStateFactory* result = new LookupVersionGlobalStateFactory();
        Federation::SerializableActivationFactory::Register(SerializableWithActivationTypes::LookupVersion, result);

        return result;
    }

    virtual Federation::SerializableWithActivation* CreateNew()
    {
        return new LookupVersionGlobalState();
    }

private:
    LookupVersionGlobalStateFactory()
    {
    }
};

Global<LookupVersionGlobalStateFactory> FactorySingleton(LookupVersionGlobalStateFactory::CreateSingleton());

LookupVersionGlobalState::LookupVersionGlobalState()
    : SerializableWithActivation(SerializableWithActivationTypes::LookupVersion), fmEndVersion_(1), fmmEndVersion_(1)
{
}

LookupVersionGlobalState::LookupVersionGlobalState(LookupVersionGlobalState const & other)
    :   SerializableWithActivation(SerializableWithActivationTypes::LookupVersion),
        fmGeneration_(other.fmGeneration_),
        fmEndVersion_(other.fmEndVersion_), 
        fmmGeneration_(other.fmmGeneration_),
        fmmEndVersion_(other.fmmEndVersion_)
{
}

bool LookupVersionGlobalState::UpdateFMState(GenerationNumber const & generation, int64 endVersion)
{
    if (generation > fmGeneration_)
    {
        fmGeneration_ = generation;
        fmEndVersion_ = endVersion;
        return true;
    }
    else if (generation == fmGeneration_ && endVersion > fmEndVersion_)
    {
        fmEndVersion_ = endVersion;
        return true;
    }

    return false;
}

bool LookupVersionGlobalState::UpdateFMMState(GenerationNumber const & generation, int64 endVersion)
{
    if (generation > fmmGeneration_)
    {
        fmmGeneration_ = generation;
        fmmEndVersion_ = endVersion;
        return true;
    }
    else if (generation == fmmGeneration_ && endVersion > fmmEndVersion_)
    {
        fmmEndVersion_ = endVersion;
        return true;
    }

    return false;
}

bool LookupVersionGlobalState::Update(LookupVersionGlobalState const & other)
{
    bool fmUpdated = UpdateFMState(other.fmGeneration_, other.fmEndVersion_);
    bool fmmUpdated = UpdateFMMState(other.fmmGeneration_, other.fmmEndVersion_);

    return fmUpdated || fmmUpdated;
}

void LookupVersionGlobalState::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("FM {0}:{1}, FMM {2}:{3}", fmGeneration_, fmEndVersion_, fmmGeneration_, fmmEndVersion_);
}

LookupVersionStoreProvider::LookupVersionStoreProvider(wstring const & traceId)
: traceId_(traceId)
{
    AddType(SerializableWithActivationTypes::LookupVersion);
}

void LookupVersionStoreProvider::AddOutputState(SerializableWithActivationList & output, NodeInstance const* target)
{
    UNREFERENCED_PARAMETER(target);

    AcquireReadLock grab(lock_);
    output.Add(make_unique<LookupVersionGlobalState>(state_));
}

void LookupVersionStoreProvider::ProcessInputState(SerializableWithActivation const & input, NodeInstance const & from)
{
    UNREFERENCED_PARAMETER(from);

    AcquireWriteLock grab(lock_);
    if (state_.Update(dynamic_cast<LookupVersionGlobalState const &>(input)))
    {
        WriteInfo(Constants::ServiceResolverSource, traceId_, "Lookup version state updated to {0}", state_);
    }
}

void LookupVersionStoreProvider::UpdateFMState(GenerationNumber const & generation, int64 endVersion)
{
    AcquireWriteLock grab(lock_);
    if (state_.UpdateFMState(generation, endVersion))
    {
        WriteInfo(Constants::ServiceResolverSource, traceId_, "FM Lookup version state updated to {0}", state_);
    }
}

void LookupVersionStoreProvider::UpdateFMMState(GenerationNumber const & generation, int64 endVersion)
{
    AcquireWriteLock grab(lock_);
    if (state_.UpdateFMMState(generation, endVersion))
    {
        WriteInfo(Constants::ServiceResolverSource, traceId_, "FMM Lookup version state updated to {0}", state_);
    }
}

LookupVersionGlobalState LookupVersionStoreProvider::GetState()
{
    AcquireReadLock grab(lock_);
    return state_;
}

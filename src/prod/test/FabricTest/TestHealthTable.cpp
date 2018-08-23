// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricTest;
using namespace TestCommon;
using namespace ServiceModel;
using namespace Management::HealthManager;
using namespace Query;

const StringLiteral TraceSource("WDTable");

const int TestEntity::MaxProperty = 2;
const int TestEntity::MaxTTL = 3600;
const int TestEntity::MinTransientEventTTL = 60;
const double TestEntity::ErrorRatio = 0.1;
const double TestEntity::WarningRatio = 0.2;

ExclusiveLock RandomLock;
Random randomGenerator;

BYTE GenerateRandomPercentage()
{
    int r = randomGenerator.Next(130);
    if (r > 100)
    {
        r = 0;
    }

    return static_cast<BYTE>(r);
}

ClusterHealthPolicy CreateClusterHealthPolicy()
{
    AcquireExclusiveLock grab(RandomLock);

    int r = randomGenerator.Next(2);
    bool considerWarningAsError = (r == 0);

    BYTE maxPercentUnhealthyNodes = GenerateRandomPercentage();
    BYTE maxPercentUnhealthyApplications = GenerateRandomPercentage();

    ClusterHealthPolicy policy(considerWarningAsError, maxPercentUnhealthyNodes, maxPercentUnhealthyApplications);
    
    r = randomGenerator.Next(5);
    if (r < 3)
    {
        if (r == 0)
        {
            // Add the app type name used in random tests
            wstring randomAppTypeName = *RandomTestApplicationHelper::AppTypeName;
            policy.AddApplicationTypeHealthPolicy(move(randomAppTypeName), GenerateRandomPercentage());
        }

        r = randomGenerator.Next(3);
        for (int i = 0; i < r; ++i)
        {
            // Generate randomGenerator app type names
            policy.AddApplicationTypeHealthPolicy(wformatString("AT-{0}", i), GenerateRandomPercentage());
        }
    }

    return policy;
}

bool GetRandomEnableDeltaHealthEvaluation()
{
    AcquireExclusiveLock grab(RandomLock);

    int r = randomGenerator.Next(2);
    return (r == 0);
}

ClusterUpgradeHealthPolicy CreateClusterUpgradeHealthPolicy()
{
    AcquireExclusiveLock grab(RandomLock);
    
    BYTE maxPercentDelta = GenerateRandomPercentage();
    BYTE maxPercentUpgradeDomainDelta = GenerateRandomPercentage();

    return ClusterUpgradeHealthPolicy(maxPercentDelta, maxPercentUpgradeDomainDelta);
}

ApplicationHealthPolicy CreateApplicationHealthPolicy()
{
    AcquireExclusiveLock grab(RandomLock);
    int r = randomGenerator.Next(2);

    bool considerWarningAsError = (r == 0);

    BYTE maxPercentUnhealthyDeployedApplications = GenerateRandomPercentage();
    BYTE maxPercentUnhealthyServices = GenerateRandomPercentage();
    BYTE maxPercentUnhealthyPartitionsPerService = GenerateRandomPercentage();
    BYTE maxPercentUnhealthyReplicasPerPartition = GenerateRandomPercentage();

    ServiceTypeHealthPolicy defaultServiceTypeHealthPolicy(maxPercentUnhealthyServices, maxPercentUnhealthyPartitionsPerService,maxPercentUnhealthyReplicasPerPartition);

    return ApplicationHealthPolicy(considerWarningAsError, maxPercentUnhealthyDeployedApplications, defaultServiceTypeHealthPolicy, ServiceTypeHealthPolicyMap());
}

vector<wstring> CreateAppNames()
{
    vector<wstring> appNames;
    int count = randomGenerator.Next(5);
    for (int i = 0; i < count; ++i)
    {
        // Generate random app names that do not exist.
        appNames.push_back(RandomFabricTestSession::GenerateAppName());
    }

    return appNames;
}

ApplicationHealthPolicyMapSPtr CreateApplicationHealthPolicyMap()
{
    {
        AcquireExclusiveLock grab(RandomLock);
        int r = randomGenerator.Next(2);
        if (r == 0)
        {
            // Do not generate a policy map
            return nullptr;
        }
    }

    auto map = make_shared<ApplicationHealthPolicyMap>();
    auto appNames = CreateAppNames();
    
    // Generate policy for fabric:/System
    bool addSystemPolicy;
    int policyCount;
    {
        AcquireExclusiveLock grab(RandomLock);
        int r = randomGenerator.Next(5);
        addSystemPolicy = (r == 0);
        policyCount = randomGenerator.Next(static_cast<int>(appNames.size()));
    }

    if (addSystemPolicy)
    {
        wstring systemApp(FABRIC_SYSTEM_APPLICATION);
        map->Insert(systemApp, CreateApplicationHealthPolicy());
    }

    // Generate policies for some of the generated apps
    for (int i = 0; i < policyCount; ++i)
    {
        map->Insert(appNames[i], CreateApplicationHealthPolicy());
    }

    return map;
}

class TestEntity::TestEvent
{
public:
    TestEvent(HealthInformation && info)
        : Info(move(info)), Instance(0), IsReported(false), IsExpired(false), Found(false), MissingTime(DateTime::Zero)
    {
    }

    bool IsMissing() const
    {
        return MissingTime != DateTime::Zero;
    }
    
    bool SetMissing()
    {
        if (!IsMissing())
        {
            MissingTime = DateTime::Now();
            return true;
        }

        return false;
    }

    bool UpdateWithHMEvent(int64 instance, HealthEvent const & hmEvent)
    {
        if (Instance == 0 && instance != 0)
        {
            TestSession::WriteNoise(TraceSource, "{0}+{1}: update instance {2} (was {3})", Info.SourceId, Info.Property, instance, Instance);
            Instance = instance;
        }

        IsReported = true;
        Found = true;
        MissingTime = DateTime::Zero;
        if (!IsExpired && hmEvent.IsExpired)
        {
            IsExpired = true;
        }

        return hmEvent.State == Info.State && hmEvent.Description == Info.Description && hmEvent.TimeToLive == Info.TimeToLive;
    }

    HealthInformation Info;
    int64 Instance;
    bool IsReported;
    bool IsExpired;
    bool Found;
    DateTime MissingTime;
};

TestEntity::TestEntity()
    :   instance_(0),
        lastInvalidatedSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
        expiredCount_(0),
        errorCount_(0),
        warningCount_(0),
        systemError_(0),
        systemWarning_(0),
        missingTime_(DateTime::Zero)
{
}

FABRIC_SEQUENCE_NUMBER TestEntity::GetLatestSequence() const
{
    FABRIC_SEQUENCE_NUMBER result = FABRIC_INVALID_SEQUENCE_NUMBER;
    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        result = max(result, (*it)->Info.SequenceNumber);
    }

    return result;
}

bool TestEntity::IsMissing() const
{
    return (missingTime_ != DateTime::Zero);
}

bool TestEntity::CanDelete() const
{
    return (missingTime_ != DateTime::Zero && missingTime_ + TimeSpan::FromMinutes(5) < DateTime::Now());
}

void TestEntity::ResetMissing()
{
    if (missingTime_ != DateTime::Zero)
    {
        TestSession::WriteInfo(TraceSource, id_, "{0}: reset missing (was {1})", id_, missingTime_);
        missingTime_ = DateTime::Zero;
    }
}

void TestEntity::SetMissing(wstring const & reason)
{
    if (missingTime_ == DateTime::Zero)
    {
        TestSession::WriteInfo(TraceSource, id_, "{0} is missing: {1}", id_, reason);
        missingTime_ = Common::DateTime::Now();
    }
}

FABRIC_HEALTH_STATE TestEntity::GetState(bool considerWarningAsError) const
{
    if (errorCount_ > 0 || systemError_ > 0)
    {
        return FABRIC_HEALTH_STATE_ERROR;
    }

    if (expiredCount_ > 0)
    {
        return FABRIC_HEALTH_STATE_ERROR;
    }

    if (warningCount_ > 0 || systemWarning_ > 0)
    {
        return (considerWarningAsError ? FABRIC_HEALTH_STATE_ERROR : FABRIC_HEALTH_STATE_WARNING);
    }

    return FABRIC_HEALTH_STATE_OK;
}

vector<TestEntity::TestEventUPtr>::iterator TestEntity::GetEvent(wstring const & source, wstring const & property)
{
    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        if ((*it)->Info.SourceId == source && (*it)->Info.Property == property)
        {
            return it;
        }
    }

    return events_.end();
}

void TestEntity::Report(TestWatchDog & watchDog)
{
    wstring property;
    bool replace = false;

    int retryCount = 0;
    int maxRetryCount = MaxProperty + 2;
    bool propertyGenerated = false;
    while(!propertyGenerated)
    {
        ++retryCount;
        property = wformatString("TestProperty{0}", watchDog.NextInt(MaxProperty));
        propertyGenerated = true;
        auto existing = GetEvent(watchDog.SourceId, property);
        if (existing != events_.end())
        {
            if (!(*existing)->IsReported && (retryCount < maxRetryCount))
            {
                // Since the event has not been validated, prefer to generate another property.
                // Otherwise, there may be race conditions like:
                // Watchdog generates R1, which is delayed because of transport.
                // Watchdog generates R2, which replaces R1 in test. HM receives and applies R2.
                // Authority system component increases entity instance, so HM removes R2.
                // HM receives (delayed) R1 and applies it, since R2 is gone and can't be used to detect staleness.
                // This fails test validation, since the test doesn't know about R1 anymore.
                propertyGenerated = false;
            }
            else
            {
                replace = true;
                events_.erase(existing);
            }
        }
    }

    FABRIC_SEQUENCE_NUMBER sequenceNumber = watchDog.GetSequence();

    wstring desc = wformatString("{0}-{1}", watchDog.SourceId, sequenceNumber);

    bool removeWhenExpired = (watchDog.NextInt(2) == 0);

    int ttlSec = watchDog.NextInt(MaxTTL) + 1;
    if (replace && removeWhenExpired && ttlSec < MinTransientEventTTL)
    {
        // For transient events, have a larger TTL to prevent races such as:
        // R1 (sn1) generated, R2 (sn2) transient w/ small TTL generated
        // Client sends first R2, applied by HM, removed because the TTL expired
        // Client sends R1, which is accepted by HM because newer event was removed due to expiration.
        // This would lead to test verification failure, because test expects HM to only have the newest event.
        // Having a larger TTL for RemoveWhenExpired ensures that the second event is correctly rejected before the newest one expires.
        ttlSec += MinTransientEventTTL;
    }
        
    TimeSpan ttl = TimeSpan::FromSeconds(ttlSec);

    FABRIC_HEALTH_STATE state;
    double temp = watchDog.NextDouble();
    if (temp < ErrorRatio)
    {
        state = FABRIC_HEALTH_STATE_ERROR;
    }
    else if (temp < ErrorRatio + WarningRatio)
    {
        state = FABRIC_HEALTH_STATE_WARNING;
    }
    else
    {
        state = FABRIC_HEALTH_STATE_OK;
    }
    
    ScopedHeap heap;
    ReferencePointer<FABRIC_HEALTH_INFORMATION> healthInformation = heap.AddItem<FABRIC_HEALTH_INFORMATION>();
    healthInformation->Description = heap.AddString(desc);
    healthInformation->SourceId = heap.AddString(watchDog.SourceId);
    healthInformation->Property = heap.AddString(property);
    healthInformation->TimeToLiveSeconds = static_cast<DWORD>(ttl.TotalSeconds());
    healthInformation->SequenceNumber = sequenceNumber;
    healthInformation->State = state;
    healthInformation->RemoveWhenExpired = removeWhenExpired;

    unique_ptr<TestEvent> info = make_unique<TestEvent>(HealthInformation(watchDog.SourceId, property, ttl, state, desc, sequenceNumber, DateTime::Now(), removeWhenExpired));
    events_.push_back(move(info));

    TestSession::WriteNoise(TraceSource, id_, "reporting: {0} {1} {2} {3} {4} {5}", watchDog.SourceId, property, state, ttl, sequenceNumber, removeWhenExpired);

    FABRIC_HEALTH_REPORT report = CreateReport(heap, healthInformation.GetRawPointer());
    watchDog.Report(report);
}

void TestEntity::UpdateCounters()
{
    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        if ((*it)->IsReported && !(*it)->Found)
        {
            (*it)->SetMissing();
        }
    }

    auto it = remove_if(events_.begin(), events_.end(), [this](TestEventUPtr const & existing) 
        {
            bool result = (existing->IsMissing() && existing->MissingTime + TimeSpan::FromMinutes(5) < DateTime::Now());
            if (result)
            {
                TestSession::WriteNoise(TraceSource, this->id_, 
                    "Removing missing event {0} {1} {2} {3} {4}",
                    this->id_, existing->Info.SourceId, existing->Info.Property, existing->Info.SequenceNumber, existing->MissingTime);
            }
            return result; 
        });

    events_.erase(it, events_.end());

    expiredCount_ = errorCount_ = warningCount_ = 0;

    for (auto eventsIter = events_.begin(); eventsIter != events_.end(); ++eventsIter)
    {
        if ((*eventsIter)->IsMissing())
        {
            continue;
        }

        if ((*eventsIter)->IsExpired)
        {
            expiredCount_++;
        }
        else if ((*eventsIter)->IsReported)
        {
            if ((*eventsIter)->Info.State == FABRIC_HEALTH_STATE_ERROR)
            {
                errorCount_++;
            }
            else if ((*eventsIter)->Info.State == FABRIC_HEALTH_STATE_WARNING)
            {
                warningCount_++;
            }
        }
    }
}

void TestEntity::UpdateExpiredEvents(std::vector<ServiceModel::HealthEvent> const & events)
{
    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        (*it)->Found = false;
    }

    for (HealthEvent const & event : events)
    {
        auto existing = GetEvent(event.SourceId, event.Property);
        if (existing != events_.end() && (*existing)->Info.SequenceNumber == event.SequenceNumber)
        {
            (*existing)->IsReported = true;
            (*existing)->Found = true;
            (*existing)->MissingTime = DateTime::Zero;
            (*existing)->IsExpired = event.IsExpired;
        }
    }

    UpdateCounters();
}

void TestEntity::UpdateInstance(shared_ptr<AttributesStoreData> const & attr, bool allowLowerInstance)
{
    FABRIC_INSTANCE_ID instance = attr->InstanceId;
    if (instance > instance_)
    {
        TestSession::WriteInfo(TraceSource, id_, "Update instance from {0} to {1}. {2}", instance_, instance, *attr);
        instance_ = instance;
        return;
    }
    
    if (instance <= 0)
    {
        // Nothing to do, not a valid instance
        return;
    }

    // More checks to investigate 8099892
    ASSERT_IFNOT(
        instance == attr->InstanceId,
        "UpdateInstance for {0}: test got instance {1} for HM, then instance {2}. {3}. Use count: {4}",
        id_,
        instance,
        attr->InstanceId,
        *attr,
        attr.use_count());

    if (instance < instance_)
    {
        ASSERT_IFNOT(
            allowLowerInstance,
            "UpdateInstance for {0}: HM instance {1} < test instance {2}. Attr instance: {3}. {4}. Use count: {5}",
            id_,
            instance,
            instance_,
            attr->InstanceId,
            *attr,
            attr.use_count());

        TestSession::WriteInfo(TraceSource, id_, "Update instance from {0} to {1}. Allow smaller instance due to rebuild. {2}", instance_, instance, *attr);
        instance_ = instance;
    }
}

void TestEntity::ResetEvents(std::wstring const & reason)
{
    if (!events_.empty())
    {
        int resetEvents = 0;
        for (auto const & event : events_)
        {
            if (event->SetMissing())
            {
                ++resetEvents;
            }
        }

        if (resetEvents > 0)
        {
            TestSession::WriteInfo(TraceSource, id_, "Reset {0} events for entity {1}: {2}", resetEvents, *this, reason);
        }
    }
}

bool TestEntity::Verify(vector<HealthEvent> const & events)
{
    set<wstring> hmEvents;
    TimeSpan cleanupInterval = Management::ManagementConfig::GetConfig().HealthStoreCleanupInterval;

    for (auto & event : events_)
    {
        event->Found = false;
    }

    systemError_ = systemWarning_ = 0;
    for (auto & hmEvent : events)
    {
        bool isSystemReport = StringUtility::StartsWith<wstring>(hmEvent.SourceId, L"System");

        if (hmEvent.IsExpired)
        {
            if (hmEvent.RemoveWhenExpired && hmEvent.LastModifiedUtcTimestamp + hmEvent.TimeToLive + cleanupInterval + cleanupInterval < DateTime::Now())
            {
                TestSession::WriteInfo(TraceSource, id_, "Expired event {0} is not deleted", hmEvent);
                return false;
            }
        }
        else if (isSystemReport)
        {
            if (hmEvent.State == FABRIC_HEALTH_STATE_ERROR)
            {
                ++systemError_;
            }
            else if (hmEvent.State == FABRIC_HEALTH_STATE_WARNING)
            {
                ++systemWarning_;
            }
        }

        if (!isSystemReport)
        {
            auto it = GetEvent(hmEvent.SourceId, hmEvent.Property);
            if (it == events_.end() || hmEvent.SequenceNumber != (*it)->Info.SequenceNumber)
            {
                TestSession::WriteInfo(TraceSource, id_, "Event {0} not found in test table: {1}", hmEvent, *this);
                DumpEvents();
                return false;
            }

            if (!(*it)->UpdateWithHMEvent(instance_, hmEvent))
            {
                TestSession::WriteInfo(TraceSource, id_, "Event {0} {1} does not match for {2}",
                    hmEvent.SourceId, hmEvent.Property, *this);
                TestSession::WriteInfo(TraceSource, id_, "HM:\t{0} {1} {2} {3}",
                    hmEvent.State, hmEvent.SequenceNumber, hmEvent.TimeToLive, hmEvent.Description);
                TestSession::WriteInfo(TraceSource, id_, "Test:\t{0} {1} {2} {3} {4}",
                    (*it)->Info.State, (*it)->Info.SequenceNumber, (*it)->Info.TimeToLive, (*it)->Info.Description, (*it)->Instance);

                return false;
            }

            hmEvents.insert(hmEvent.SourceId + L"." + hmEvent.Property);
        }
    }

    for (auto & event : events_)
    {
        if ((!event->IsMissing()) &&
            (event->Instance >= instance_) &&
            (event->Info.SequenceNumber > lastInvalidatedSequence_) &&
            (!event->Info.RemoveWhenExpired || event->Info.SourceUtcTimestamp + event->Info.TimeToLive > DateTime::Now()) &&
            (hmEvents.find(event->Info.SourceId + L"." + event->Info.Property) == hmEvents.end()))
        {
            TestSession::WriteInfo(
                TraceSource,
                id_,
                "Event {0} {1} {2} {3} {4} {5} not found on HM. Event instance {6}, current instance {7}",
                event->Info.SourceId,
                event->Info.Property,
                event->Info.RemoveWhenExpired,
                event->Info.SequenceNumber,
                event->Instance,
                event->IsExpired,
                event->Instance,
                instance_);
            return false;
        }
    }

    UpdateCounters();

    return true;
}

bool TestEntity::VerifyChildrenResult(EntityStateResults const & queryResults, EntityStateResults & children) const
{
    for (auto itResults = queryResults.begin(); itResults != queryResults.end(); ++itResults)
    {
        auto it = children.find(itResults->first);
        if (it == children.end())
        {
            TestSession::WriteInfo(
                TraceSource,
                id_,
                "Unexpected child {0}:{1} for {2} in results", 
                itResults->first, itResults->second, id_);
            return false;
        }

        if (it->second != itResults->second)
        {
            TestSession::WriteInfo(
                TraceSource, 
                id_,
                "AggregatedHealthState for child {0} expected {1} actual {2} for {3} in results", 
                itResults->first, it->second, itResults->second, id_);
            return false;
        }

        children.erase(it);
    }

    if (children.size() > 0)
    {
        TestSession::WriteInfo(
            TraceSource, 
            id_,
            "Child {0} not found for {1} in query results", 
            children.begin()->first, id_);
        return false;
    }

    return true;
}

bool TestEntity::VerifyQueryResult(std::wstring const & policyString, FABRIC_HEALTH_STATE state, EntityStateResults & children, EntityStateResults const & queryChildrenStates, std::vector<ServiceModel::HealthEvent> const & events, FABRIC_HEALTH_STATE aggregatedHealthState) const
{
    wstring childrenString = wformatString(children);
    bool result = VerifyChildrenResult(queryChildrenStates, children);

    if (result && state != aggregatedHealthState)
    {
        TestSession::WriteInfo(
            TraceSource, 
            id_,
            "AggregatedHealthState for {0} expected {1} actual {2}", 
            id_, state, aggregatedHealthState);
        result = false;
    }

    if (result)
    {
        TestSession::WriteNoise(
            TraceSource, 
            id_,
            "QueryResult: {0}\r\n,Entity: {1}, state: {2}", 
            id_, *this, state);
    }
    else
    {
        TestSession::WriteWarning(
            TraceSource, 
            id_,
            "QueryResult: {0}\r\n,Entity: {1}, children: {2}\r\nPolicy: {3}", 
            id_, *this, childrenString, policyString);

        DumpEvents();

        for (HealthEvent const & entityEvent : events)
        {
            TestSession::WriteInfo(
                TraceSource, 
                id_,
                "Event: {0}", 
                entityEvent);
        }

        for (auto it = queryChildrenStates.begin(); it != queryChildrenStates.end(); ++it)
        {
            TestSession::WriteInfo(
                TraceSource, 
                id_,
                "Child: {0} {1}", 
                it->first, it->second);
        }
    }

    return result;
}

void TestEntity::SetInvalidatedSequence()
{
    lastInvalidatedSequence_ = GetLatestSequence();
}

void TestEntity::WriteTo(__in Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("Error={0}+{1}, Warning={2}+{3}, expired={4}, instance={5} ",
        errorCount_, systemError_, warningCount_, systemWarning_, expiredCount_, instance_);
}

void TestEntity::DumpEvents() const
{
    for (auto it = events_.begin(); it != events_.end(); ++it)
    {
        string flags = formatString("{0}{1}{2}",
            (*it)->Info.RemoveWhenExpired,
            (*it)->IsReported ? "" : " NotReported",
            (*it)->IsExpired ? " Expired" : "");

        TestSession::WriteInfo(
            TraceSource, 
            id_,
            "TableEvent: {0} {1} {2} {3} {4} {5} {6} {7}",
            (*it)->Info.SourceId + L" " + (*it)->Info.Property,
            (*it)->Info.State,
            (*it)->Info.TimeToLive,
            (*it)->Info.SequenceNumber,
            (*it)->Info.SourceUtcTimestamp,
            (*it)->Info.Description,
            (*it)->Instance,
            flags);
    }
}

template <class T, class H, class P>
class EntityTable
{
public:
    typedef H HealthEntityType;

    EntityTable()
        : lock_()
        , entities_()
    {
    }

    int GetReportableCount() const
    {
        int result = 0;
        auto entities = this->GetEntities();
        for (auto const & entry : entities)
        {
            if (entry->IsReportable())
            {
                ++result;
            }
        }

        return result;
    }

    shared_ptr<T> GetEntity(std::wstring const & id) const
    {
        AcquireReadLock grab(lock_);
        auto it = entities_.find(id);
        if (it == entities_.end())
        {
            return nullptr;
        }

        return it->second;
    }

    vector<shared_ptr<T>> GetEntities() const
    {
        AcquireReadLock grab(lock_);
        vector<shared_ptr<T>> result;
        for (auto it = entities_.begin(); it != entities_.end(); ++it)
        {
            result.push_back(it->second);
        }

        return result;
    }

    void Report(TestWatchDog & watchDog, int index)
    {
        int i = 0;

        auto entities = this->GetEntities();
        for (auto const & entry : entities)
        {
            if (entry->IsReportable())
            {
                if (i++ == index)
                {
                    entry->Report(watchDog);
                    return;
                }
            }
        }
    }
    
    bool TestQuery(TestHealthTable const & table, ComPointer<IFabricHealthClient4> const & healthClient, set<wstring> & verifiedIds)
    {
        bool result = true;
        
        map<wstring, shared_ptr<T>> entries;
        { // lock
            AcquireReadLock grab(lock_);
            for (auto const & entry : entities_)
            {
                if (entry.second->IsMissing() || !entry.second->CanQuery() || verifiedIds.find(entry.first) != verifiedIds.end())
                {
                    continue;
                }

                entries.insert(make_pair(entry.first, entry.second));
            }
        } //endlock

        for (auto const & entry : entries)
        {
            if (verifiedIds.find(entry.first) != verifiedIds.end())
            {
                continue;
            }

            TestSession::WriteNoise(TraceSource, entry.first, "TestQuery for {0}", entry.first);

            wstring policyString;
            HRESULT hr = TestFabricClient::PerformFabricClientOperation(
                L"HealthQuery." + entry.first,
                FabricTestSessionConfig::GetConfig().NamingOperationTimeout,
                [&](DWORD const timeout, ComPointer<ComCallbackWaiter> const & callback, ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return entry.second->BeginTestQuery(healthClient, TimeSpan::FromMilliseconds(timeout), callback.GetRawPointer(), context.InitializationAddress(), policyString);
            },
                [&](ComPointer<IFabricAsyncOperationContext> & context) -> HRESULT
            {
                return entry.second->EndTestQuery(table, healthClient, context.GetRawPointer(), policyString);
            },
                S_OK,
                S_OK,
                S_OK,
                false);

            if (SUCCEEDED(hr))
            {
                TestSession::WriteNoise(TraceSource, entry.first, "Health Query for {0} succeeded.", entry.first);
                verifiedIds.insert(entry.first);
            }
            else
            {
                TestSession::WriteWarning(TraceSource, entry.first, "Health Query for {0} returned {1}", entry.first, hr);
                result = false;
            }
        }

        return result;
    }

    void UpdateEntities(vector<HealthEntitySPtr> const & entities, __out vector<shared_ptr<H>> & castedEntities)
    {
        AcquireWriteLock grab(lock_);

        map<wstring, shared_ptr<T>> tmp;
        entities_.swap(tmp);

        for (auto const & entity : entities)
        {
            auto attr = entity->GetAttributesCopy();
            if (attr->IsMarkedForDeletion || (!attr->HasSystemReport) || attr->IsCleanedUp)
            {
                continue;
            }

            auto it = tmp.find(entity->EntityIdString);
            shared_ptr<T> entry;
            if (it != tmp.end())
            {
                entry = move(it->second);
                tmp.erase(it);
            }
            else
            {
                entry = make_shared<T>();
            }

            auto castedEntity = static_pointer_cast<H>(entity);
            entry->Update(castedEntity, attr);
            castedEntities.push_back(move(castedEntity));

            if (entity->Test_HasHierarchySystemReport())
            {
                entry->ResetMissing();
            }
            else
            {
                entry->SetMissing(L"no system report");
                if (entity->State.IsPendingFirstReport)
                {
                    // The entity is cleaned up or doesn't have any reports.
                    // This can happen when entity is cleaned up due to invalidation from FM rebuild.
                    // Since the entity can be recreated, clear the test events so validation can see the correct information.
                    entry->ResetEvents(L"no system report and pending first report");
                }
            }

            entities_.insert(make_pair(entity->EntityIdString, move(entry)));
        }

        for (auto it = tmp.begin(); it != tmp.end(); ++it)
        {
            if (it->second->CanDelete())
            {
                TestSession::WriteInfo(TraceSource, it->first, "Removing entity {0}", *(it->second));
            }
            else
            {
                it->second->SetMissing(L"no HM entity");
                // Since the entity doesn't exist on HM, all events associated with it are gone
                it->second->ResetEvents(L"no HM entity");
                entities_.insert(make_pair(it->first, move(it->second)));
            }
        }
    }

    void UpdateEvents(vector<shared_ptr<H>> const & entities)
    {
        for (shared_ptr<H> const & entity : entities)
        {
            shared_ptr<AttributesStoreData> attribute;
            vector<HealthEvent> events;
            HealthEntityState::Enum entityState;
            if (entity->Test_GetData(entityState, attribute, events))
            {
                Verify(entity, events, false);
            }
        }
    }

    bool Verify(shared_ptr<H> const & entity, vector<HealthEvent> const & events, bool isInvalidated)
    {
        if (isInvalidated)
        {
            AcquireWriteLock grab(lock_);
            return VerifyCallerHoldsLock(entity, events, isInvalidated);
        }
        else
        {
            AcquireReadLock grab(lock_);
            return VerifyCallerHoldsLock(entity, events, isInvalidated);
        }
    }

    vector<shared_ptr<T>> GetChildren(std::wstring const & parentId)
    {
        // Get entities under the lock, then analyze them outside the lock
        auto entities = this->GetEntities();

        vector<shared_ptr<T>> result;
        for (auto && entity : entities)
        {
            if (!entity->IsMissing() && entity->GetParentId() == parentId && entity->IsParentHealthy())
            {
                result.push_back(move(entity));
            }
        }

        return result;
    }

    FABRIC_HEALTH_STATE GetChildrenState(TestHealthTable const & table, std::wstring const & parentId, BYTE maxPercentUnhealthy, P const & policy, __out EntityStateResults & results)
    {
        map<wstring, shared_ptr<T>> entries;
        { // lock
            AcquireReadLock grab(lock_);
            for (auto const & entry : entities_)
            {
                if (!entry.second->IsMissing() && entry.second->GetParentId() == parentId)
                {
                    entries.insert(make_pair(entry.first, entry.second));
                }
            }
        } //endlock

        HealthCount count;
        for (auto & entry : entries)
        {
            if (entry.second->IsParentHealthy())
            {
                EntityStateResults subResults;
                FABRIC_HEALTH_STATE state = entry.second->GetAggregatedState(table, policy, subResults);
                results[entry.first] = state;
                count.AddResult(state);
            }
        }

        return count.GetHealthState(maxPercentUnhealthy);
    }

private:
    bool VerifyCallerHoldsLock(shared_ptr<H> const & entity, vector<HealthEvent> const & events, bool isInvalidated)
    {
        wstring const & id = entity->EntityIdString;

        auto it = entities_.find(id);
        if (it != entities_.end())
        {
            if (isInvalidated)
            {
                it->second->SetInvalidatedSequence();
            }

            bool result = it->second->Verify(events);
            TestSession::WriteNoise(TraceSource, id, "Verifying {0} with {1} events result {2}: {3}", id, events.size(), result, *(it->second));
            return result;
        }
        else
        {
            TestSession::WriteWarning(TraceSource, id, "Verifying {0} not found in table", id);
        }

        return true;
    }

protected:
    mutable RwLock lock_;
    map<wstring, shared_ptr<T>> entities_;
};

class TestNodeEntity : public TestEntity
{
public:
    TestNodeEntity()
        : nodeName_()
        , upgradeDomain_()
    {
    }

    void Update(NodeEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;

        auto attr = dynamic_cast<NodeAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);

        nodeName_ = attr->NodeName;
        upgradeDomain_ = attr->UpgradeDomain;
        FABRIC_INSTANCE_ID instance = attrBase->InstanceId;
        int64 derivedInstance = attr->InstanceId;
        ASSERT_IF(instance != derivedInstance, "The attrBase instance {0} is not equal to the attr instance {1}.\r\nAttr={2},\r\nattrBase={3}", instance, derivedInstance, *attr, *attrBase);
        
        FABRIC_NODE_INSTANCE_ID nodeInstance = attr->NodeInstanceId;
        ASSERT_IF(
            static_cast<FABRIC_NODE_INSTANCE_ID>(instance) != nodeInstance,
            "The attrBase instance {0} is not equal to the node instance {1}, {2}",
            instance,
            nodeInstance,
            *attr);
        
        bool isRebuildRandom = (FABRICSESSION.Label == L"RebuildRandom" || FABRICSESSION.Label == L"RebuildNDataRandom");
        UpdateInstance(attrBase, isRebuildRandom);
    }

    wstring GetUpgradeDomain() const
    {
        return upgradeDomain_;
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ClusterHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        UNREFERENCED_PARAMETER(table);
        UNREFERENCED_PARAMETER(results);
        return GetState(policy.ConsiderWarningAsError);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ClusterHealthPolicy policy = CreateClusterHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto nodeHealthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *nodeHealthPolicy);
        return healthClient->BeginGetNodeHealth(nodeName_.c_str(), nodeHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricNodeHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetNodeHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        NodeHealth health;
        auto error = health.FromPublicApi(*healthResult->get_NodeHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        ClusterHealthPolicy policy;
        error = ClusterHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, EntityStateResults(), health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("nodename={0}, instance={1}",
            nodeName_, instance_);
    }

private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto nodeHealthInformation = heap.AddItem<FABRIC_NODE_HEALTH_REPORT>();
        nodeHealthInformation->NodeName = heap.AddString(nodeName_);
        nodeHealthInformation->HealthInformation = commonHealthInformation;
    
        FABRIC_HEALTH_REPORT result;
        result.Value = nodeHealthInformation.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_NODE;

        return result;
    }

    wstring nodeName_;
    wstring upgradeDomain_;
};

class NodeEntityTable : public EntityTable<TestNodeEntity, NodeEntity, ClusterHealthPolicy>
{
public:
    vector<wstring> GetUpgradeDomains()
    {
        vector<wstring> results;
        auto entities = this->GetEntities();

        for (auto const & entry : entities)
        {
            if (entry->IsMissing())
            {
                continue;
            }

            wstring upgradeDomain = entry->GetUpgradeDomain();
            if (upgradeDomain.empty())
            {
                continue;
            }

            if (find(results.begin(), results.end(), upgradeDomain) == results.end())
            {
                results.push_back(upgradeDomain);
            }
        }

        sort(results.begin(), results.end());
        return results;
    }
};

Global<NodeEntityTable> Nodes = make_global<NodeEntityTable>();

class TestServiceEntity : public TestEntity
{
public:
    TestServiceEntity()
    {
    }

    wstring const & GetServiceTypeName()
    {
        return serviceTypeName_;
    }

    virtual wstring GetParentId() const
    {
        return appName_;
    }

    void Update(ServiceEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;

        auto attr = dynamic_cast<ServiceAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);

        appName_ = attr->ApplicationName;
        serviceTypeName_ = attr->ServiceTypeName;
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        FABRIC_HEALTH_STATE result = GetState(policy.ConsiderWarningAsError);
        auto serviceTypePolicy = policy.GetServiceTypeHealthPolicy(serviceTypeName_);
        FABRIC_HEALTH_STATE childrenResult = table.GetPartitionsState(id_, serviceTypePolicy.MaxPercentUnhealthyPartitionsPerService, policy, results);

        return max(result, childrenResult);
    }

    virtual bool CanQuery() const
    {
        return (appName_ != SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        return healthClient->BeginGetServiceHealth(id_.c_str(), applicationHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricServiceHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetServiceHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        ServiceHealth health;
        auto error = health.FromPublicApi(*healthResult->get_ServiceHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        EntityStateResults queryChildrenStates;
        for (auto it = health.PartitionsAggregatedHealthStates.begin(); it != health.PartitionsAggregatedHealthStates.end(); ++it)
        {
            wstring childEntityId = wformatString("{0}", it->PartitionId);
            queryChildrenStates[childEntityId] = it->AggregatedHealthState;
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, queryChildrenStates, health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("appName={0}, serviceTypeName={1}",
            appName_, serviceTypeName_);
    }

private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto serviceInformation = heap.AddItem<FABRIC_SERVICE_HEALTH_REPORT>();
        serviceInformation->ServiceName = heap.AddString(id_);
        serviceInformation->HealthInformation = commonHealthInformation;
    
        FABRIC_HEALTH_REPORT result;
        result.Value = serviceInformation.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_SERVICE;

        return result;
    }

    wstring appName_;
    wstring serviceTypeName_;
};

Global<EntityTable<TestServiceEntity, ServiceEntity, ApplicationHealthPolicy>> Services = make_global<EntityTable<TestServiceEntity, ServiceEntity, ApplicationHealthPolicy>>();

class TestPartitionEntity : public TestEntity
{
public:
    TestPartitionEntity()
    {
    }

    void Update(PartitionEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;

        auto attr = dynamic_cast<PartitionAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);

        partitionId_ = attr->EntityId;
        serviceName_ = attr->ServiceName;
    }

    wstring const & GetServiceName()
    {
        return serviceName_;
    }

    virtual wstring GetParentId() const
    {
        return serviceName_;
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        FABRIC_HEALTH_STATE result = GetState(policy.ConsiderWarningAsError);
        auto service = Services->GetEntity(serviceName_);
        auto serviceTypePolicy = policy.GetServiceTypeHealthPolicy(service->GetServiceTypeName());
        FABRIC_HEALTH_STATE childrenResult = table.GetReplicasState(id_, serviceTypePolicy.MaxPercentUnhealthyReplicasPerPartition, policy, results);

        return max(result, childrenResult);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        return healthClient->BeginGetPartitionHealth(partitionId_.AsGUID(), applicationHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricPartitionHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetPartitionHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        PartitionHealth health;
        auto error = health.FromPublicApi(*healthResult->get_PartitionHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        EntityStateResults queryChildrenStates;
        for (auto it = health.ReplicasAggregatedHealthStates.begin(); it != health.ReplicasAggregatedHealthStates.end(); ++it)
        {
            wstring childEntityId = wformatString("{0}+{1}", it->PartitionId, it->ReplicaId);
            queryChildrenStates[childEntityId] = it->AggregatedHealthState;
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, queryChildrenStates, health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("service={0}", serviceName_);
    }

private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto partitionHealthReport = heap.AddItem<FABRIC_PARTITION_HEALTH_REPORT>();
        partitionHealthReport->PartitionId = partitionId_.AsGUID();
        partitionHealthReport->HealthInformation = commonHealthInformation;
    
        FABRIC_HEALTH_REPORT result;
        result.Value = partitionHealthReport.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_PARTITION;

        return result;
    }

    Guid partitionId_;
    wstring serviceName_;
};

Global<EntityTable<TestPartitionEntity, PartitionEntity, ApplicationHealthPolicy>> Partitions = make_global<EntityTable<TestPartitionEntity, PartitionEntity, ApplicationHealthPolicy>>();

class TestDeployedServicePackageEntity : public TestEntity
{
public:
    TestDeployedServicePackageEntity()
    {
    }

    wstring const & GetServiceManifestName() const
    {
        return servicePackageName_;
    }

    wstring const & GetServicePackageActivationId() const
    {
        return servicePackageActivationId_;
    }

    FABRIC_NODE_INSTANCE_ID GetNodeInstance() const
    {
        return nodeInstance_;
    }

    virtual wstring GetParentId() const
    {
        return wformatString("{0}+{1}", appName_, nodeId_);
    }

    void Update(DeployedServicePackageEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;

        auto attr = dynamic_cast<DeployedServicePackageAttributesStoreData*>(attrBase.get());

        nodeName_ = attr->NodeName;
        nodeInstance_ = attr->NodeInstanceId;
        appName_ = attr->EntityId.ApplicationName;
        servicePackageName_ = attr->EntityId.ServiceManifestName;
        servicePackageActivationId_ = attr->EntityId.ServicePackageActivationId;
        nodeId_ = attr->EntityId.NodeId;
        int64 instance = attrBase->InstanceId;
        int64 derivedInstance = attr->InstanceId;
        ASSERT_IF(instance != derivedInstance, "The attrBase instance {0} is not equal to the attr instance {1}.\r\nAttr={2},\r\nattrBase={3}", instance, derivedInstance, *attr, *attrBase);

        UpdateInstance(attrBase, false);
    }

    virtual bool IsParentHealthy() const
    {
        auto node = Nodes->GetEntity(nodeId_.ToString());
        return (node && !node->HasSystemError() && node->GetInstance() == static_cast<int64>(nodeInstance_));
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        UNREFERENCED_PARAMETER(table);
        UNREFERENCED_PARAMETER(results);
        return GetState(policy.ConsiderWarningAsError);
    }

    virtual bool CanReport() const
    {
        return (appName_.size() > 0 && appName_ != SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    virtual bool CanQuery() const
    {
        return (appName_.size() > 0);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;

        auto healthQueryDescription = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION>();
        healthQueryDescription->ApplicationName = heap.AddString(appName_);
        healthQueryDescription->ServiceManifestName = heap.AddString(servicePackageName_);
        healthQueryDescription->NodeName = heap.AddString(nodeName_);

        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        healthQueryDescription->HealthPolicy = applicationHealthPolicy.GetRawPointer();

        auto healthQueryDescriptionEx1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_QUERY_DESCRIPTION_EX1>();
        healthQueryDescriptionEx1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
        healthQueryDescription->Reserved = healthQueryDescriptionEx1.GetRawPointer();

        return healthClient->BeginGetDeployedServicePackageHealth2(
            healthQueryDescription.GetRawPointer(),
            static_cast<DWORD>(timeout.TotalMilliseconds()), 
            callback, 
            context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricDeployedServicePackageHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetDeployedServicePackageHealth2(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        DeployedServicePackageHealth health;
        auto error = health.FromPublicApi(*healthResult->get_DeployedServicePackageHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, EntityStateResults(), health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("instance={0}, nodeinstance={1}",
            instance_, nodeInstance_);
    }

private:
    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto deployedServicePackageHealthInformation = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT>();
        deployedServicePackageHealthInformation->NodeName = heap.AddString(nodeName_);
        deployedServicePackageHealthInformation->ApplicationName = heap.AddString(appName_);
        deployedServicePackageHealthInformation->ServiceManifestName = heap.AddString(servicePackageName_);
        deployedServicePackageHealthInformation->HealthInformation = commonHealthInformation;

        auto ex1 = heap.AddItem<FABRIC_DEPLOYED_SERVICE_PACKAGE_HEALTH_REPORT_EX1>();
        ex1->ServicePackageActivationId = heap.AddString(servicePackageActivationId_);
        deployedServicePackageHealthInformation->Reserved = ex1.GetRawPointer();
    
        FABRIC_HEALTH_REPORT result;
        result.Value = deployedServicePackageHealthInformation.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_DEPLOYED_SERVICE_PACKAGE;

        return result;
    }

    wstring appName_;
    wstring servicePackageName_;
    wstring servicePackageActivationId_;
    NodeHealthId nodeId_;
    wstring nodeName_;
    FABRIC_NODE_INSTANCE_ID nodeInstance_;
};

Global<EntityTable<TestDeployedServicePackageEntity, DeployedServicePackageEntity, ApplicationHealthPolicy>> DeployedServicePackages = make_global<EntityTable<TestDeployedServicePackageEntity, DeployedServicePackageEntity, ApplicationHealthPolicy>>();

class TestReplicaEntity : public TestEntity
{
public:
    TestReplicaEntity()
    {
        instance_ = -2;
    }

    virtual wstring GetParentId() const
    {
        return partitionId_.ToString();
    }

    void Update(ReplicaEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;
        
        auto attr = dynamic_cast<ReplicaAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);

        partitionId_ = attr->EntityId.PartitionId;
        replicaId_ = attr->EntityId.ReplicaId;
        nodeInstance_ = attr->NodeInstanceId;
        nodeId_ = attr->NodeId;
        int64 instance = attrBase->InstanceId;
        int64 derivedInstance = attr->InstanceId;
        ASSERT_IF(instance != derivedInstance, "The attrBase instance {0} is not equal to the attr instance {1}.\r\nAttr={2},\r\nattrBase={3}", instance, derivedInstance, *attr, *attrBase);

        if (!attr->UseInstance)
        {
            ASSERT_IF(instance > 0, "HM stateless instance has instance set: {0}", *attr);
        }
        else
        {
            UpdateInstance(attrBase, false);
        }
    }

    virtual bool IsParentHealthy() const
    {
        auto node = Nodes->GetEntity(nodeId_.ToString());
        if (!node || node->HasSystemError() || node->GetInstance() != static_cast<int64>(nodeInstance_))
        {
            TestSession::WriteInfo(TraceSource, id_, "rejecting replica {0} node {1}", *this, *node);
            return false;
        }

        auto partition = Partitions->GetEntity(partitionId_.ToString());
        if (!partition)
        {
            TestSession::WriteInfo(TraceSource, id_, "rejecting replica {0} partition", *this);
            return false;
        }

        return true;
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        UNREFERENCED_PARAMETER(table);
        UNREFERENCED_PARAMETER(results);
        return GetState(policy.ConsiderWarningAsError);
    }

    virtual bool CanQuery() const
    {
        return !FABRICSESSION.FabricDispatcher.IsRebuildLostFailoverUnit(partitionId_);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        return healthClient->BeginGetReplicaHealth(partitionId_.AsGUID(), replicaId_, applicationHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricReplicaHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetReplicaHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        ReplicaHealth health;
        auto error = health.FromPublicApi(*healthResult->get_ReplicaHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, EntityStateResults(), health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("node={0}:{1}",
            nodeId_, nodeInstance_);
    }

private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        FABRIC_HEALTH_REPORT result;

        if (instance_ == -2)
        {
            auto replicaHealthReport = heap.AddItem<FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_REPORT>();
            replicaHealthReport->PartitionId = partitionId_.AsGUID();
            replicaHealthReport->InstanceId = replicaId_;
            replicaHealthReport->HealthInformation = commonHealthInformation;
    
            result.Value = replicaHealthReport.GetRawPointer();
            result.Kind = FABRIC_HEALTH_REPORT_KIND_STATELESS_SERVICE_INSTANCE;
        }
        else
        {
            auto replicaHealthReport = heap.AddItem<FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_REPORT>();
            replicaHealthReport->PartitionId = partitionId_.AsGUID();
            replicaHealthReport->ReplicaId = replicaId_;
            replicaHealthReport->HealthInformation = commonHealthInformation;
    
            result.Value = replicaHealthReport.GetRawPointer();
            result.Kind = FABRIC_HEALTH_REPORT_KIND_STATEFUL_SERVICE_REPLICA;
        }

        return result;
    }

    Guid partitionId_;
    FABRIC_REPLICA_ID replicaId_;
    NodeHealthId nodeId_;
    FABRIC_NODE_INSTANCE_ID nodeInstance_;
};

Global<EntityTable<TestReplicaEntity, ReplicaEntity, ApplicationHealthPolicy>> Replicas = make_global<EntityTable<TestReplicaEntity, ReplicaEntity, ApplicationHealthPolicy>>();

class TestDeployedApplicationEntity : public TestEntity
{
public:
    TestDeployedApplicationEntity()
    {
    }

    NodeHealthId GetNodeId() const
    {
        return nodeId_;
    }

    FABRIC_NODE_INSTANCE_ID GetNodeInstance() const
    {
        return nodeInstance_;
    }

    virtual bool IsParentHealthy() const
    {
        auto node = Nodes->GetEntity(nodeId_.ToString());
        return (node && !node->HasSystemError() && node->GetInstance() == static_cast<int64>(nodeInstance_));
    }

    virtual wstring GetParentId() const
    {
        return appName_;
    }

    virtual bool CanReport() const
    {
        return (appName_.size() > 0 && appName_ != SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    void Update(DeployedApplicationEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;
        
        auto attr = dynamic_cast<DeployedApplicationAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);
        
        appName_ = attr->EntityId.ApplicationName;
        nodeId_ = attr->EntityId.NodeId;
        nodeName_ = attr->NodeName;
        nodeInstance_ = attr->NodeInstanceId;
        int64 instance = attrBase->InstanceId;
        int64 derivedInstance = attr->InstanceId;
        ASSERT_IF(instance != derivedInstance, "The attrBase instance {0} is not equal to the attr instance {1}.\r\nAttr={2},\r\nattrBase={3}", instance, derivedInstance, *attr, *attrBase);

        UpdateInstance(attrBase, false);
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        FABRIC_HEALTH_STATE result = GetState(policy.ConsiderWarningAsError);
        FABRIC_HEALTH_STATE childrenResult = table.GetDeployedServicePackagesState(id_, 0, policy, results);

        return max(result, childrenResult);
    }

    virtual bool CanQuery() const
    {
        return (appName_.size() > 0);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        return healthClient->BeginGetDeployedApplicationHealth(appName_.c_str(), nodeName_.c_str(), applicationHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricDeployedApplicationHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetDeployedApplicationHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        DeployedApplicationHealth health;
        auto error = health.FromPublicApi(*healthResult->get_DeployedApplicationHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        EntityStateResults queryChildrenStates;
        for (auto it = health.DeployedServicePackagesAggregatedHealthStates.begin(); it != health.DeployedServicePackagesAggregatedHealthStates.end(); ++it)
        {
            NodeId nodeId;
            error = NodeIdGenerator::GenerateFromString(it->NodeName, nodeId);
            if (!error.IsSuccess())
            {
                Assert::CodingError("Can't generate node id for {0}: {1}", it->NodeName, error);
            }

            //
            // This key generation should match with DeployedServicePackageHealthId::ToString().
            //
            wstring childEntityId = wformatString("{0}+{1}+{2}", it->ApplicationName, it->ServiceManifestName, nodeId);

            if (!(it->ServicePackageActivationId.empty()))
            {
                childEntityId = wformatString("{0}+{1}+{2}+{3}", it->ApplicationName, it->ServiceManifestName, it->ServicePackageActivationId, nodeId);
            }
            
            queryChildrenStates[childEntityId] = it->AggregatedHealthState;
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, queryChildrenStates, health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }


private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto deployedApplicationHealthInformation = heap.AddItem<FABRIC_DEPLOYED_APPLICATION_HEALTH_REPORT>();
        deployedApplicationHealthInformation->NodeName = heap.AddString(nodeName_);
        deployedApplicationHealthInformation->ApplicationName = heap.AddString(appName_);
        deployedApplicationHealthInformation->HealthInformation = commonHealthInformation;
    
        FABRIC_HEALTH_REPORT result;
        result.Value = deployedApplicationHealthInformation.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_DEPLOYED_APPLICATION;

        return result;
    }

    wstring appName_;
    NodeHealthId nodeId_;
    wstring nodeName_;
    FABRIC_NODE_INSTANCE_ID nodeInstance_;
};

Global<EntityTable<TestDeployedApplicationEntity, DeployedApplicationEntity, ApplicationHealthPolicy>> DeployedApplications = make_global<EntityTable<TestDeployedApplicationEntity, DeployedApplicationEntity, ApplicationHealthPolicy>>();

class TestApplicationEntity : public TestEntity
{
public:
    TestApplicationEntity() 
        : appName_()
        , appType_()
        , policy_()
    {
    }

    wstring const & GetAppType() const
    {
        return appType_;
    }

    shared_ptr<ApplicationHealthPolicy> GetPolicy() const
    {
        return policy_;
    }

    void Update(ApplicationEntitySPtr const & entity, AttributesStoreDataSPtr const & attrBase)
    {
        id_ = entity->EntityIdString;

        auto error = entity->GetApplicationHealthPolicy(policy_);
        if (!error.IsSuccess())
        {
            policy_.reset();
        }

        auto attr = dynamic_cast<ApplicationAttributesStoreData*>(attrBase.get());
        ASSERT_IFNOT(attr, "Attributes dynamic_cast returned null for {0}", *attrBase);

        appName_ = attr->EntityId.ApplicationName;
        int64 instance = attrBase->InstanceId;
        int64 derivedInstance = attr->InstanceId;
        ASSERT_IF(instance != derivedInstance, "The attrBase instance {0} is not equal to the attr instance {1}.\r\nAttr={2},\r\nattrBase={3}", instance, derivedInstance, *attr, *attrBase);

        UpdateInstance(attrBase, false);

        appType_ = attr->ApplicationTypeName;
    }

    FABRIC_HEALTH_STATE GetAggregatedState(TestHealthTable const & table, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
    {
        FABRIC_HEALTH_STATE result = GetState(policy.ConsiderWarningAsError);
        FABRIC_HEALTH_STATE servicesResult = table.GetServicesState(id_, policy, results);
        FABRIC_HEALTH_STATE deployedApplicationResult = table.GetDeployedApplicationsState(id_, policy.MaxPercentUnhealthyDeployedApplications, policy, results);

        return max(result, max(servicesResult, deployedApplicationResult));
    }

    virtual bool CanQuery() const
    {
        return (appName_.size() > 0 && appName_ != SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    virtual bool CanReport() const
    {
        return (appName_.size() > 0 && appName_ != SystemServiceApplicationNameHelper::SystemServiceApplicationName);
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto applicationHealthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *applicationHealthPolicy);
        return healthClient->BeginGetApplicationHealth(id_.c_str(), applicationHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricApplicationHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetApplicationHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        ApplicationHealth health;
        auto error = health.FromPublicApi(*healthResult->get_ApplicationHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        EntityStateResults queryChildrenStates;
        for (auto it = health.ServicesAggregatedHealthStates.begin(); it != health.ServicesAggregatedHealthStates.end(); ++it)
        {
            queryChildrenStates[it->ServiceName] = it->AggregatedHealthState;
        }

        for (auto it = health.DeployedApplicationsAggregatedHealthStates.begin(); it != health.DeployedApplicationsAggregatedHealthStates.end(); ++it)
        {
            NodeId nodeId;
            error = NodeIdGenerator::GenerateFromString(it->NodeName, nodeId);
            if (!error.IsSuccess())
            {
                Assert::CodingError("Can't generate node id for {0}: {1}", it->NodeName, error);
            }

            wstring childEntityId = wformatString("{0}+{1}", it->ApplicationName, nodeId);
            queryChildrenStates[childEntityId] = it->AggregatedHealthState;
        }

        ApplicationHealthPolicy policy;
        error = ApplicationHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        UpdateExpiredEvents(health.Events);

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);

        if (!VerifyQueryResult(policyString, state, children, queryChildrenStates, health.Events, health.AggregatedHealthState))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    bool TestIsApplicationHealthy(TestHealthTable const & table, vector<wstring> const & upgradeDomains)
    {
        HealthManagerReplicaSPtr hm = FABRICSESSION.FabricDispatcher.GetHM();
        if (!hm)
        {
            TestSession::WriteInfo(TraceSource, "HM is not ready");
            return false;
        }

        ApplicationHealthPolicy policy = CreateApplicationHealthPolicy();

        EntityStateResults children;
        FABRIC_HEALTH_STATE state = GetAggregatedState(table, policy, children);
        bool expectedIsHealthy = (state != FABRIC_HEALTH_STATE_ERROR);

        vector<wstring> upgradedDomains;
        ActivityId activityId(Guid::NewGuid());

        for (size_t i = 0; i <= upgradeDomains.size(); i++)
        {
            bool actualIsHealthy;
            vector<HealthEvaluation> unhealthyEvaluations;
            ErrorCode error = hm->IsApplicationHealthy(activityId, appName_, upgradedDomains, &policy, actualIsHealthy, unhealthyEvaluations);

            if (!error.IsSuccess())
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    id_,
                    "IsApplicationHealthy for {0} returned: {1}", 
                    appName_, error);
                return false;
            }

            if (actualIsHealthy != expectedIsHealthy)
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    id_,
                    "TestIsApplicationHealthy for {0} expected {1} actual {2}\r\n{3}\r\npolicy: {4}\r\nUpgraded domains: {5}", 
                    appName_, expectedIsHealthy, actualIsHealthy, children, policy, upgradedDomains);

                DumpEvents();
            }

            if (actualIsHealthy)
            {
                if (!unhealthyEvaluations.empty())
                {
                    TestSession::FailTest(
                        "TestIsApplicationHealthy for {0} returned healthy and evaluation reasons {1}", 
                        appName_, unhealthyEvaluations[0].Evaluation->Description);
                }
            }
            else
            {
                if (unhealthyEvaluations.size() != 1)
                {
                    TestSession::FailTest(
                        "TestIsApplicationHealthy for {0} returned unhealthy and {1} evaluation reasons", 
                        appName_, unhealthyEvaluations.size());
                }

                switch (unhealthyEvaluations[0].Evaluation->Kind)
                {
                case FABRIC_HEALTH_EVALUATION_KIND_EVENT:
                case FABRIC_HEALTH_EVALUATION_KIND_SERVICES:
                case FABRIC_HEALTH_EVALUATION_KIND_DEPLOYED_APPLICATIONS:
                case FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DEPLOYED_APPLICATIONS:
                    // Acceptable reasons
                    break;
                default:
                    TestSession::FailTest("Unexpected evaluation reason for application {0}: {1}",
                        appName_, unhealthyEvaluations[0].Evaluation->Kind);
                    break;
                }
            }

            if (i < upgradeDomains.size())
            {
                upgradedDomains.push_back(upgradeDomains[i]);

                if (expectedIsHealthy)
                {
                    EntityStateResults domainChildren;
                    FABRIC_HEALTH_STATE deployedApplicationResult = table.GetDeployedApplicationsState(id_, policy.MaxPercentUnhealthyDeployedApplications, policy, domainChildren, upgradeDomains[i]);
                    expectedIsHealthy = (deployedApplicationResult != FABRIC_HEALTH_STATE_ERROR);
                }
            }
        }

        return true;
    }

    void WriteTo(__in Common::TextWriter& w, Common::FormatOptions const & format) const
    {
        TestEntity::WriteTo(w, format);
        w.Write("appname={0}, instance={1}, appType={2}", appName_, instance_, appType_);
    }

private:

    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap & heap, FABRIC_HEALTH_INFORMATION *commonHealthInformation)
    {
        auto applicationInformation = heap.AddItem<FABRIC_APPLICATION_HEALTH_REPORT>();
        applicationInformation->ApplicationName = heap.AddString(appName_);
        applicationInformation->HealthInformation = commonHealthInformation;
    
        FABRIC_HEALTH_REPORT result;
        result.Value = applicationInformation.GetRawPointer();
        result.Kind = FABRIC_HEALTH_REPORT_KIND_APPLICATION;

        return result;
    }

    wstring appName_;
    wstring appType_;
    shared_ptr<ApplicationHealthPolicy> policy_;
};

class ApplicationEntityTable : public EntityTable<TestApplicationEntity, ApplicationEntity, ApplicationHealthPolicy>
{
public:
    bool TestIsApplicationHealthy(TestHealthTable const & table, vector<wstring> const & upgradeDomains)
    {
        AcquireReadLock grab(lock_);
        for (auto it = entities_.begin(); it != entities_.end(); ++it)
        {
            if (!it->second->IsMissing() && !it->second->TestIsApplicationHealthy(table, upgradeDomains))
            {
                return false;
            }
        }

        return true;
    }
};

Global<ApplicationEntityTable> Applications = make_global<ApplicationEntityTable>();

class TestClusterEntity : public TestEntity
{
public:
    TestClusterEntity()
    {
    }

    virtual bool CanReport() const
    {
        return false;
    }

    virtual HRESULT BeginTestQuery(
        ComPointer<IFabricHealthClient4> const & healthClient,
        TimeSpan timeout,
        IFabricAsyncOperationCallback *callback,
        IFabricAsyncOperationContext **context,
        wstring & policyString) 
    {
        ClusterHealthPolicy policy = CreateClusterHealthPolicy();
        policyString = policy.ToString();

        ScopedHeap heap;
        auto clusterHealthPolicy = heap.AddItem<FABRIC_CLUSTER_HEALTH_POLICY>();
        policy.ToPublicApi(heap, *clusterHealthPolicy);
        return healthClient->BeginGetClusterHealth(clusterHealthPolicy.GetRawPointer(), static_cast<DWORD>(timeout.TotalMilliseconds()), callback, context);
    }

    virtual HRESULT EndTestQuery(
        TestHealthTable const & table,
        ComPointer<IFabricHealthClient4> const & healthClient,
        IFabricAsyncOperationContext *context,
        wstring const & policyString) 
    {
        ComPointer<IFabricClusterHealthResult> healthResult;

        HRESULT hr = healthClient->EndGetClusterHealth(context, healthResult.InitializationAddress());
        if (!SUCCEEDED(hr))
        {
            return hr;
        }

        ClusterHealth health;
        auto error = health.FromPublicApi(*healthResult->get_ClusterHealth());
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        ClusterHealthPolicy policy;
        error = ClusterHealthPolicy::FromString(policyString, policy);
        if (!error.IsSuccess())
        {
            return error.ToHResult();
        }

        if (!table.TestClusterHealth(health, policy))
        {
            return E_FAIL;
        }

        return S_OK;
    }

    void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const
    {
    }
    
private:
    virtual FABRIC_HEALTH_REPORT CreateReport(ScopedHeap &, FABRIC_HEALTH_INFORMATION *)
    {
        Assert::CodingError("Should not be called");
    }
};

class ClusterEntityTable : public EntityTable<TestClusterEntity, ClusterEntity, ClusterHealthPolicy>
{
public:
    ClusterEntityTable()
    {
        entities_.insert(make_pair(L"", make_unique<TestClusterEntity>()));
    }
};

Global<ClusterEntityTable> Cluster = make_global<ClusterEntityTable>();

TestHealthTable::TestHealthTable()
    : lock_()
    , active_(true)
{
}

wstring TestHealthTable::GetUpgradeDomain(std::wstring const & nodeId) const
{
    auto node = Nodes->GetEntity(nodeId);
    return (node != nullptr ? node->GetUpgradeDomain() : L"");
}

void TestHealthTable::Report(TestWatchDog & watchDog)
{
    AcquireWriteLock grab(lock_);
    if (!active_)
    {
        return;
    }

    int count1 = Nodes->GetReportableCount();
    int count2 = count1 + Partitions->GetReportableCount();
    int count3 = count2 + Replicas->GetReportableCount();
    int count4 = count3 + DeployedServicePackages->GetReportableCount();
    int count5 = count4 + DeployedApplications->GetReportableCount();
    int count6 = count5 + Services->GetReportableCount();
    int count7 = count6 + Applications->GetReportableCount();

    TestSession::WriteNoise(TraceSource, "counts: {0} {1} {2} {3} {4} {5} {6}", count1, count2, count3, count4, count5, count6, count7);

    if (count7 == 0)
    {
        return;
    }

    int r = watchDog.NextInt(count7);
    if (r < count1)
    {
        Nodes->Report(watchDog, r);
    }
    else if (r < count2)
    {
        Partitions->Report(watchDog, r - count1);
    }
    else if (r < count3)
    {
        Replicas->Report(watchDog, r - count2);
    }
    else if (r < count4)
    {
        DeployedServicePackages->Report(watchDog, r - count3);
    }
    else if (r < count5)
    {
        DeployedApplications->Report(watchDog, r - count4);
    }
    else if (r < count6)
    {
        Services->Report(watchDog, r - count5);
    }
    else 
    {
        Applications->Report(watchDog, r - count6);
    }
}

void TestHealthTable::Pause()
{
    AcquireWriteLock grab(lock_);
    active_ = false;
}

void TestHealthTable::Resume()
{
    AcquireWriteLock grab(lock_);
    active_ = true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<NodeEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmNodes;
    ErrorCode error = hm->EntityManager->Nodes.GetEntities(ActivityId(), hmNodes);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get nodes failed: {0}", error);
        return false;
    }

    // Get the list of entities that are not marked for deletion and have system report
    Nodes->UpdateEntities(hmNodes, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<PartitionEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmPartitions;
    ErrorCode error = hm->EntityManager->Partitions.GetEntities(ActivityId(), hmPartitions);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get partitions failed: {0}", error);
        return false;
    }

    Partitions->UpdateEntities(hmPartitions, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<ReplicaEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmReplicas;
    ErrorCode error = hm->EntityManager->Replicas.GetEntities(ActivityId(), hmReplicas);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get replicas failed: {0}", error);
        return false;
    }

    Replicas->UpdateEntities(hmReplicas, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<DeployedServicePackageEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmDeployedServicePackages;
    ErrorCode error = hm->EntityManager->DeployedServicePackages.GetEntities(ActivityId(), hmDeployedServicePackages);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get deployed service packages failed: {0}", error);
        return false;
    }

    DeployedServicePackages->UpdateEntities(hmDeployedServicePackages, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<ServiceEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmServices;
    ErrorCode error = hm->EntityManager->Services.GetEntities(ActivityId(), hmServices);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get services failed: {0}", error);
        return false;
    }

    Services->UpdateEntities(hmServices, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<DeployedApplicationEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmDeployedApplications;
    ErrorCode error = hm->EntityManager->DeployedApplications.GetEntities(ActivityId(), hmDeployedApplications);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get deployed applications failed: {0}", error);
        return false;
    }

    DeployedApplications->UpdateEntities(hmDeployedApplications, entities);
    return true;
}

bool TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, __out vector<ApplicationEntitySPtr> & entities)
{
    vector<HealthEntitySPtr> hmApplications;
    ErrorCode error = hm->EntityManager->Applications.GetEntities(ActivityId(), hmApplications);
    if (!error.IsSuccess())
    {
        TestSession::WriteInfo(TraceSource, "HM get applications failed: {0}", error);
        return false;
    }

    Applications->UpdateEntities(hmApplications, entities);
    return true;
}

bool TestHealthTable::Verify(NodeEntitySPtr const & entity, vector<HealthEvent> const & events, bool isInvalidated)
{
    return Nodes->Verify(entity, events, isInvalidated);
}

bool TestHealthTable::Verify(PartitionEntitySPtr const & entity, vector<HealthEvent> const & events, bool isInvalidated)
{
    return Partitions->Verify(entity, events, isInvalidated);
}

bool TestHealthTable::Verify(ReplicaEntitySPtr const & entity, vector<HealthEvent> const & events)
{
    return Replicas->Verify(entity, events, false);
}

bool TestHealthTable::Verify(ServiceEntitySPtr const & entity, vector<HealthEvent> const & events, bool isInvalidated)
{
    return Services->Verify(entity, events, isInvalidated);
}

bool TestHealthTable::Verify(ApplicationEntitySPtr const & entity, vector<HealthEvent> const & events)
{
    return Applications->Verify(entity, events, false);
}

bool TestHealthTable::Verify(DeployedServicePackageEntitySPtr const & entity, vector<HealthEvent> const & events)
{
    return DeployedServicePackages->Verify(entity, events, false);
}

bool TestHealthTable::Verify(DeployedApplicationEntitySPtr const & entity, vector<HealthEvent> const & events)
{
    return DeployedApplications->Verify(entity, events, false);
}

FABRIC_HEALTH_STATE TestHealthTable::GetReplicasState(wstring const & parentId, BYTE maxPercentUnhealthy, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
{
    return Replicas->GetChildrenState(*this, parentId, maxPercentUnhealthy, policy, results);
}

FABRIC_HEALTH_STATE TestHealthTable::GetPartitionsState(wstring const & parentId, BYTE maxPercentUnhealthy, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
{
    return Partitions->GetChildrenState(*this, parentId, maxPercentUnhealthy, policy, results);
}

FABRIC_HEALTH_STATE TestHealthTable::GetDeployedServicePackagesState(wstring const & parentId, BYTE maxPercentUnhealthy, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
{
    auto children = DeployedServicePackages->GetChildren(parentId);

    HealthCount count;
    for (auto const & child : children)
    {
        EntityStateResults subResults;
        FABRIC_HEALTH_STATE state = child->GetAggregatedState(*this, policy, subResults);
        results[child->GetId()] = state;
        count.AddResult(state);
    }

    return count.GetHealthState(maxPercentUnhealthy);
}

FABRIC_HEALTH_STATE TestHealthTable::GetServicesState(wstring const & parentId, ApplicationHealthPolicy const & policy, __out EntityStateResults & results) const
{
    auto children = Services->GetChildren(parentId);

    map<wstring, HealthCount> serviceTypeCounts;
    for (auto const & child : children)
    {
        EntityStateResults subResults;
        FABRIC_HEALTH_STATE state = child->GetAggregatedState(*this, policy, subResults);
        results[child->GetId()] = state;

        auto it = serviceTypeCounts.find(child->GetServiceTypeName());
        if (it == serviceTypeCounts.end())
        {
            auto r = serviceTypeCounts.insert(make_pair(child->GetServiceTypeName(), HealthCount()));
            it = r.first;
        }

        it->second.AddResult(state);
    }

    FABRIC_HEALTH_STATE state = FABRIC_HEALTH_STATE_OK;
    for (auto it = serviceTypeCounts.begin(); it != serviceTypeCounts.end(); ++it)
    {
        BYTE maxPercentUnhealthy;
        auto policyIt = policy.ServiceTypeHealthPolicies.find(it->first);
        if (policyIt != policy.ServiceTypeHealthPolicies.end())
        {
            maxPercentUnhealthy = policyIt->second.MaxPercentUnhealthyServices;
        }
        else
        {
            maxPercentUnhealthy = policy.DefaultServiceTypeHealthPolicy.MaxPercentUnhealthyServices;
        }

        FABRIC_HEALTH_STATE serviceTypeState = it->second.GetHealthState(maxPercentUnhealthy);
        if (serviceTypeState > state)
        {
            state = serviceTypeState;
        }
    }

    return state;
}

FABRIC_HEALTH_STATE TestHealthTable::GetDeployedApplicationsState(wstring const & parentId, BYTE maxPercentUnhealthy, ApplicationHealthPolicy const & policy, __out EntityStateResults & results, std::wstring const & upgradeDomain) const
{
    auto children = DeployedApplications->GetChildren(parentId);

    HealthCount count;
    for (auto const & child: children)
    {
        if (upgradeDomain.size() == 0 || GetUpgradeDomain(child->GetNodeId().ToString()) == upgradeDomain)
        {
            EntityStateResults subResults;
            FABRIC_HEALTH_STATE state = child->GetAggregatedState(*this, policy, subResults);
            results[child->GetId()] = state;
            count.AddResult(state);
        }
    }

    return count.GetHealthState(maxPercentUnhealthy);
}

FABRIC_HEALTH_STATE TestHealthTable::GetNodesState(BYTE maxPercentUnhealthy, ClusterHealthPolicy const & policy, __out EntityStateResults & results, std::wstring const & upgradeDomain) const
{
    auto nodes = Nodes->GetEntities();

    HealthCount count;
    for (auto const & node : nodes)
    {
        if (upgradeDomain.empty() || GetUpgradeDomain(node->GetId()) == upgradeDomain)
        {
            EntityStateResults subResults;
            FABRIC_HEALTH_STATE state = node->GetAggregatedState(*this, policy, subResults);
            results[node->GetId()] = state;
            count.AddResult(state);
        }
    }

    return count.GetHealthState(maxPercentUnhealthy);
}

FABRIC_HEALTH_STATE TestHealthTable::GetApplicationsState(BYTE maxPercentUnhealthy, ApplicationTypeHealthPolicyMap const & appTypePolicyMap, ApplicationHealthPolicyMapSPtr const & appHealthPolicyMap, __out EntityStateResults & results) const
{
    auto applications = Applications->GetEntities();
    FABRIC_HEALTH_STATE systemApplicationState = FABRIC_HEALTH_STATE_UNKNOWN;

    HealthCount count;
    
    map<wstring, HealthCount> appTypeCounts;
    if (CommonConfig::GetConfig().EnableApplicationTypeHealthEvaluation)
    {
        for (auto it = appTypePolicyMap.begin(); it != appTypePolicyMap.end(); ++it)
        {
            appTypeCounts.insert(make_pair(it->first, HealthCount()));
        }
    }

    for (auto const & application : applications)
    {
        wstring const & appName = application->GetId();

        // Look for app policy inside the app health policy map first.
        ApplicationHealthPolicySPtr policy;
        if (appHealthPolicyMap)
        {
            policy = appHealthPolicyMap->GetApplicationHealthPolicy(appName);
        }

        if (!policy)
        {
            policy = application->GetPolicy();
        }

        if (policy)
        {
            EntityStateResults subResults;
            FABRIC_HEALTH_STATE state = application->GetAggregatedState(*this, *policy, subResults);
            if (appName == SystemServiceApplicationNameHelper::SystemServiceApplicationName)
            {
                systemApplicationState = state;
                TestSession::WriteInfo(
                    TraceSource, 
                    "System application state {0}: {1}", 
                    state, subResults);
            }
            else
            {
                results[appName] = state;
                if (appName.empty())
                {
                    count.AddResult(state); // ad-hoc app
                }
                else
                {
                    wstring const & appType = application->GetAppType();
                    TestSession::FailTestIf(appType.empty(), "AppType is not set for app {0}", *application);
                    auto itAppTypeCounts = appTypeCounts.find(appType);
                    if (itAppTypeCounts == appTypeCounts.end())
                    {
                        // add to global
                        count.AddResult(state);
                    }
                    else
                    {
                        itAppTypeCounts->second.AddResult(state);
                    }
                }
            }
        }
    }

    FABRIC_HEALTH_STATE aggrState = max(count.GetHealthState(maxPercentUnhealthy), systemApplicationState);

    if (!appTypeCounts.empty())
    {
        auto itTypeCounts = appTypeCounts.begin();
        for (auto it = appTypePolicyMap.begin(); it != appTypePolicyMap.end(); ++it, ++itTypeCounts)
        {
            aggrState = max(aggrState, itTypeCounts->second.GetHealthState(it->second));
        }
    }

    return aggrState;
}

bool TestHealthTable::TestClusterHealth(ClusterHealth const & health, ClusterHealthPolicy const & policy) const
{
    EntityStateResults nodes;
    FABRIC_HEALTH_STATE nodesState = GetNodesState(policy.MaxPercentUnhealthyNodes, policy, nodes);

    // Create an empty app health policy map
    ApplicationHealthPolicyMapSPtr apphealthPolicyMap;

    EntityStateResults applications;
    FABRIC_HEALTH_STATE applicationsState = GetApplicationsState(policy.MaxPercentUnhealthyApplications, policy.ApplicationTypeMap, apphealthPolicyMap, applications);

    FABRIC_HEALTH_STATE expected = max(nodesState, applicationsState);

    if (health.AggregatedHealthState != expected)
    {
        TestSession::WriteWarning(
            TraceSource, 
            "ClusterHealth expected {0} actual {1}, policy {2}", 
            expected, health.AggregatedHealthState, policy);

        TestSession::WriteInfo(
            TraceSource, 
            "Nodes {0}: {1}\r\napplications {2}: {3}", 
            nodesState, nodes, applicationsState, applications);
        return false;
    }

    return true;
}

bool TestHealthTable::TestIsClusterHealthy(vector<wstring> const & upgradeDomains) const
{
    HealthManagerReplicaSPtr hm = FABRICSESSION.FabricDispatcher.GetHM();
    if (!hm)
    {
        TestSession::WriteInfo(TraceSource, "HM is not ready");
        return false;
    }

    shared_ptr<ClusterHealthPolicy> healthPolicy = make_shared<ClusterHealthPolicy>(CreateClusterHealthPolicy());
    ClusterHealthPolicy const & policy = *healthPolicy;

    auto applicationHealthPolicies = CreateApplicationHealthPolicyMap();

    EntityStateResults nodes;
    FABRIC_HEALTH_STATE nodesState = GetNodesState(policy.MaxPercentUnhealthyNodes, policy, nodes);

    EntityStateResults applications;
    FABRIC_HEALTH_STATE applicationsState = GetApplicationsState(policy.MaxPercentUnhealthyApplications, policy.ApplicationTypeMap, applicationHealthPolicies, applications);
    bool expectedIsHealthy = (max(nodesState, applicationsState) != FABRIC_HEALTH_STATE_ERROR);

    vector<wstring> upgradedDomains;
    ActivityId activityId(Guid::NewGuid());

    bool isDeltaEnabled = GetRandomEnableDeltaHealthEvaluation();
    ClusterUpgradeStateSnapshot baseline;
    shared_ptr<ClusterUpgradeHealthPolicy> upgradeHealthPolicy;
    if (isDeltaEnabled)
    {
        upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(CreateClusterUpgradeHealthPolicy());
        auto error = hm->GetClusterUpgradeSnapshot(activityId, baseline);
        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "{0}: GetClusterUpgradeSnapshot returned {1}", activityId, error);
            return false;
        }

        TestSession::WriteNoise(TraceSource, "{0}: IsClusterHealthy with deltas enabled: {1}, baseline={2}", activityId, *upgradeHealthPolicy, baseline);
    }
    else
    {
        TestSession::WriteNoise(TraceSource, "{0}: IsClusterHealthy with deltas disabled", activityId);
    }
    
    for (size_t i = 0; i <= upgradeDomains.size(); i++)
    {
        bool actualIsHealthy;
        vector<HealthEvaluation> unhealthyEvaluations;
        vector<wstring> actualAppsWithoutAppType;
        ErrorCode error = hm->Test_IsClusterHealthy(activityId, upgradedDomains, healthPolicy, upgradeHealthPolicy, applicationHealthPolicies, baseline, actualIsHealthy, unhealthyEvaluations, actualAppsWithoutAppType);

        if (!error.IsSuccess())
        {
            TestSession::WriteWarning(TraceSource, "{0}: IsClusterHealthy returned {1}", activityId, error);
            return false;
        }

        if (actualIsHealthy != expectedIsHealthy)
        {
            if (isDeltaEnabled)
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "{0}: TestIsClusterHealthy expected {1} actual {2}\r\n{3}\r\n{4}\r\npolicy: {5}\r\nupgradepolicy: {6}\r\nbaseline: {7}\r\nUpgraded domains: {8}",
                    activityId, expectedIsHealthy, actualIsHealthy, nodes, applications, policy, *upgradeHealthPolicy, baseline, upgradedDomains);
            }
            else
            {
                TestSession::WriteWarning(
                    TraceSource,
                    "TestIsClusterHealthy expected {0} actual {1}\r\n{2}\r\n{3}\r\npolicy: {4}\r\nUpgraded domains: {5}",
                    expectedIsHealthy, actualIsHealthy, nodes, applications, policy, upgradedDomains);
            }
        }

        if (actualIsHealthy)
        {
            if (!unhealthyEvaluations.empty())
            {
                TestSession::FailTest("{0}: TestIsClusterHealthy returned healthy and evaluation reasons {1}", activityId, unhealthyEvaluations[0].Evaluation->Description);
            }
        }
        else
        {
            if (unhealthyEvaluations.size() != 1)
            {
                TestSession::FailTest("{0}: TestIsClusterHealthy returned unhealthy and {1} evaluation reasons", activityId, unhealthyEvaluations.size());
            }

            vector<FABRIC_HEALTH_EVALUATION_KIND> possibleHealthEvaluations;
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_EVENT);
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_APPLICATIONS);

            if (!policy.ApplicationTypeMap.empty())
            {
                possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_APPLICATION_TYPE_APPLICATIONS);
            }

            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_NODES);
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_NODES);
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_SYSTEM_APPLICATION);
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_DELTA_NODES_CHECK);
            possibleHealthEvaluations.push_back(FABRIC_HEALTH_EVALUATION_KIND_UPGRADE_DOMAIN_DELTA_NODES_CHECK);

            TestFabricClientHealth::PrintHealthEvaluations(unhealthyEvaluations, possibleHealthEvaluations);
        }

        if (i < upgradeDomains.size())
        {
            upgradedDomains.push_back(upgradeDomains[i]);

            if (expectedIsHealthy)
            {
                if (isDeltaEnabled)
                {
                    ClusterUpgradeStateSnapshot current;
                    error = hm->GetClusterUpgradeSnapshot(activityId, current);
                    if (!error.IsSuccess())
                    {
                        TestSession::WriteWarning(TraceSource, "{0}: GetClusterUpgradeSnapshot returned {1}", activityId, error);
                        return false;
                    }

                    if (!baseline.IsGlobalDeltaRespected(current.GlobalUnhealthyState.ErrorCount, current.GlobalUnhealthyState.TotalCount, upgradeHealthPolicy->MaxPercentDeltaUnhealthyNodes))
                    {
                        expectedIsHealthy = false;
                    }
                    else
                    {
                        // for all completed upgrade domains, check per ud delta
                        for (auto const & ud : upgradedDomains)
                        {
                            ULONG baseError, baseTotal, currentError, currentTotal;
                            error = baseline.TryGetUpgradeDomainEntry(ud, baseError, baseTotal);
                            if (!error.IsSuccess())
                            {
                                TestSession::WriteWarning(TraceSource, "{0}: ud {1}: baseline.TryGetUpgradeDomainEntry returned {2}", activityId, ud, error);
                                return false;
                            }

                            error = current.TryGetUpgradeDomainEntry(ud, currentError, currentTotal);
                            if (!error.IsSuccess())
                            {
                                TestSession::WriteWarning(TraceSource, "{0}: ud {1}: currrent.TryGetUpgradeDomainEntry returned {2}", activityId, ud, error);
                                return false;
                            }

                            if (!ClusterUpgradeStateSnapshot::IsDeltaRespected(baseError, baseTotal, currentError, currentTotal, upgradeHealthPolicy->MaxPercentUpgradeDomainDeltaUnhealthyNodes))
                            {
                                expectedIsHealthy = false;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    EntityStateResults domainChildren;
                    FABRIC_HEALTH_STATE nodeResult = GetNodesState(policy.MaxPercentUnhealthyNodes, policy, domainChildren, upgradeDomains[i]);
                    expectedIsHealthy = (nodeResult != FABRIC_HEALTH_STATE_ERROR);
                }
            }
        }
    }

    return true;
}

bool TestHealthTable::RunQuery(ComPointer<IFabricHealthClient4> const & healthClient, set<wstring> & verifiedIds)
{
    AcquireReadLock grab(lock_);

    bool result = Nodes->TestQuery(*this, healthClient, verifiedIds);
    result = result && Replicas->TestQuery(*this, healthClient, verifiedIds);
    result = result && Partitions->TestQuery(*this, healthClient, verifiedIds);
    result = result && Services->TestQuery(*this, healthClient, verifiedIds);
    result = result && DeployedServicePackages->TestQuery(*this, healthClient, verifiedIds);
    result = result && DeployedApplications->TestQuery(*this, healthClient, verifiedIds);
    result = result && Applications->TestQuery(*this, healthClient, verifiedIds);
    result = result && Cluster->TestQuery(*this, healthClient, verifiedIds);

    if (result)
    {
        vector<wstring> upgradeDomains = Nodes->GetUpgradeDomains();
        result = Applications->TestIsApplicationHealthy(*this, upgradeDomains);

        if (result)
        {
            result = TestIsClusterHealthy(upgradeDomains);
        }
    }

    return result;
}

template<class TE>
void TestHealthTable::Update(HealthManagerReplicaSPtr const & hm, TE & entityTable)
{
    vector<shared_ptr<typename TE::HealthEntityType>> entities;
    Update(hm, entities);
    entityTable.UpdateEvents(entities);
}

void TestHealthTable::Update()
{
    HealthManagerReplicaSPtr hm = FABRICSESSION.FabricDispatcher.GetHM();
    if (hm)
    {
        Update(hm, *Nodes);
        Update(hm, *Partitions);
        Update(hm, *Replicas);
        Update(hm, *Services);
        Update(hm, *Applications);
        Update(hm, *DeployedServicePackages);
        Update(hm, *DeployedApplications);
    }
}

bool TestHealthTable::TestQuery(ComPointer<IFabricHealthClient4> const & healthClient)
{
    bool enableLoadBalancing = Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled;
    if(enableLoadBalancing)
    {
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled = false; 
    }

    bool result = false;
    
    int64 maxRetryMsec = Management::ManagementConfig::GetConfig().HealthStoreCleanupInterval.TotalMilliseconds() * 3;
    int i = 0;

    Stopwatch watch;
    watch.Start();

    set<wstring> verifiedIds;

    do
    {
        if (i == 0)
        {
            TestSession::WriteInfo(TraceSource, "Running health query...");
        }
        else
        {
            TestSession::WriteInfo(TraceSource, "Retrying health query, elapsed {0} msec, max {1} msec, try#{2}...", watch.ElapsedMilliseconds, maxRetryMsec, i);
        }
        
        if (RunQuery(healthClient, verifiedIds))
        {
            result = true;
            break;
        }
        
        Update();
    }
    while ((++i < 5) || (watch.ElapsedMilliseconds < maxRetryMsec));

    if(enableLoadBalancing)
    {
        Reliability::LoadBalancingComponent::PLBConfig::GetConfig().LoadBalancingEnabled = true;
    }

    if (!result)
    {
        TestSession::FailTest("Health query verification failed");
    }

    return result;
}

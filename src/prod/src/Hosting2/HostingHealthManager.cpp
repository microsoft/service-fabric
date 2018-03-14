// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;
using namespace Client;

StringLiteral const TraceType("HostingHealthManager");

class HostingHealthManager::NodeEntityManager :
public RootedObject,
public FabricComponent,
public TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(NodeEntityManager)

public:
    NodeEntityManager(
        Common::ComponentRoot const & root,
        __in HostingSubsystem & hosting)
        : RootedObject(root),
        hosting_(hosting),
        healthClient_(),
        registeredComponents_(),
        lock_(),
        isClosed_(false)
    {
    }

    ~NodeEntityManager() {}

    void InitializeHealthReportingComponent(HealthReportingComponentSPtr const & healthClient)
    {
        healthClient_ = healthClient;
    }

    Common::ErrorCode RegisterSource(wstring const & componentId)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");
        {
            AcquireWriteLock lock(lock_);
            if(isClosed_)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            auto componentIter = registeredComponents_.find(componentId);
            ASSERT_IF(
                componentIter != registeredComponents_.end(),
                "ComponentId:{0} is already registered",
                componentId);

            registeredComponents_.insert(make_pair(componentId, false));            
        }

        WriteNoise(
            TraceType,
            Root.TraceId,
            "RegisterSource: ComponentId:{0}.",
            componentId);

        return ErrorCodeValue::Success;
    }

    void UnregisterSource(wstring const & componentId)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");

        FABRIC_SEQUENCE_NUMBER healthSequenceNumber;
        bool componentReportedHealth = false;
        {
            AcquireWriteLock lock(lock_);

            if(isClosed_)
            {
                // Object is closed
                return;
            }

            auto componentIter = registeredComponents_.find(componentId);
            if(componentIter == registeredComponents_.end())
            {
                WriteNoise(
                    TraceType,
                    Root.TraceId,
                    "UnregisterSource: ComponentId:{0} is not registered.",
                    componentId);
                return;
            }

            componentReportedHealth = componentIter->second;

            registeredComponents_.erase(componentIter);            

            healthSequenceNumber = SequenceNumber::GetNext();
        }

        WriteNoise(
            TraceType,
            Root.TraceId,
            "UnregisterSource: ComponentId:{0} is unregistered.",
            componentId);

        if(componentReportedHealth)
        {
            this->DeleteHealthEvent(
                componentId,
                healthSequenceNumber);
        }
    }

    void ReportHealth(
        wstring const & componentId,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");

        {
            AcquireReadLock lock(lock_);

            if(isClosed_) { return; }

            auto componentIter = registeredComponents_.find(componentId);
            if(componentIter == registeredComponents_.end())
            {
                WriteNoise(
                    TraceType,
                    Root.TraceId,
                    "ReportHealth: Component is not registered for the entity. ComponentId:{0}",
                    componentId);
                return;
            }

            componentIter->second = true;
        }

        this->ReportHealthEvent(componentId, reportCode, healthDescription, sequenceNumber, TimeSpan::MaxValue);
    }

    void ReportHealthWithTTL(
        wstring const & componentId,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER sequenceNumber,
        Common::TimeSpan const timeToLive)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty.");

        {
            AcquireReadLock lock(lock_);

            if(isClosed_) { return; }

            auto componentIter = registeredComponents_.find(componentId);
            ASSERT_IF(registeredComponents_.find(componentId) != registeredComponents_.end(), 
                "'{0}' is reporting health with TTL. The ComponentId should not be registered since the lifetime management of the event is done based on TTL.",
                componentId);
        }

        this->ReportHealthEvent(componentId, reportCode, healthDescription, sequenceNumber, timeToLive);
    }

protected:
    virtual Common::ErrorCode OnOpen()
    {
        ASSERT_IF(!this->healthClient_, "HealthClient must be initialized before calling open.");
        {
            AcquireWriteLock lock(this->lock_);
            isClosed_ = false;
        }

        return ErrorCodeValue::Success;
    }

    virtual Common::ErrorCode OnClose()
    {
        map<wstring, bool> map;
        {
            AcquireWriteLock lock(this->lock_);
            map = registeredComponents_;
            registeredComponents_.clear();
            isClosed_ = true;
        }

        for(auto iter = map.begin(); iter != map.end(); ++iter)
        {
            // Delete only if health is reported for this component
            if(iter->second)
            {
                this->DeleteHealthEvent(
                    iter->first,
                    SequenceNumber::GetNext());
            }            
        }

        return ErrorCodeValue::Success;
    }

    virtual void OnAbort()
    {
        this->OnClose().ReadValue();
    }

private:
    void ReportHealthEvent(
        wstring const & propertyName,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber,
        Common::TimeSpan const timeToLive)
    {
        auto entityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            hosting_.NodeInstanceId);
        AttributeList attributeList;
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, hosting_.NodeName);

        auto healthReport = HealthReport::CreateSystemHealthReport(
            reportCode,
            move(entityInfo),
            propertyName,
            healthDescription,
            healthSequenceNumber,
            timeToLive,
            move(attributeList));

        WriteInfo(
            TraceType,
            Root.TraceId,
            "Node ReportHealth: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }

    void DeleteHealthEvent(
        wstring const & propertyName,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {
        auto entityInfo = EntityHealthInformation::CreateNodeEntityHealthInformation(
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            hosting_.NodeInstanceId);
        AttributeList attributeList;
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, hosting_.NodeName);

        auto healthReport = HealthReport::CreateSystemRemoveHealthReport(
            SystemHealthReportCode::Hosting_DeleteEvent,
            move(entityInfo),
            propertyName,
            healthSequenceNumber);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "Node DeleteEvent: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }

private:
    HostingSubsystem & hosting_;
    HealthReportingComponentSPtr healthClient_;

    map<wstring, bool> registeredComponents_;
    Common::RwLock lock_;
    bool isClosed_;
};

template <class TEntity>
class HostingHealthManager::EntityManagerBase :   
    public RootedObject,
    public FabricComponent,
    public TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(EntityManagerBase)

public:
    EntityManagerBase(
        Common::ComponentRoot const & root,
        __in HostingSubsystem & hosting)
        : RootedObject(root),
        hosting_(hosting),
        healthClient_(),
        entityMap_(),
        lock_(),
        isClosed_(false)
    {
    }

    ~EntityManagerBase() {}

    void InitializeHealthReportingComponent(HealthReportingComponentSPtr const & healthClient)
    {
        healthClient_ = healthClient;
    }

    Common::ErrorCode RegisterSource(TEntity const & entityId, wstring applicationName, wstring const & componentId)
    {
        ASSERT_IF(applicationName.empty(), "ApplicationName cannot be empty. EntityId: {0}", entityId);
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty. EntityId: {0}", entityId);

        {
            AcquireWriteLock lock(lock_);

            if(isClosed_)
            {
                return ErrorCodeValue::ObjectClosed;
            }

            EntitySPtr entity;
            auto entityIter = entityMap_.find(entityId);
            if(entityIter == entityMap_.end())
            {
                entity = make_shared<Entity>(entityId, applicationName);
                entityMap_.insert(make_pair(entityId, entity));

                WriteInfo(        
                    TraceType,
                    Root.TraceId,
                    "RegisterSource: Entity created. Entity={0}",
                    *entity);
            }
            else
            {
                entity = entityIter->second;
            }

            auto componentIter = entity->RegisteredComponents.find(componentId);    
            ASSERT_IF(
                componentIter != entity->RegisteredComponents.end(), 
                "ComponentId:{0} is already registered", 
                componentId);                

            entity->RegisteredComponents.insert(make_pair(componentId, false));

            WriteNoise(        
                TraceType,
                Root.TraceId,
                "RegisterSource: ComponentId:{0}. Entity:{1}",
                componentId,
                *entity);
        }

        return ErrorCodeValue::Success;
    }

    void UnregisterSource(TEntity const & entityId, wstring const & componentId)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty. EntityId: {0}", entityId);

        FABRIC_SEQUENCE_NUMBER healthSequenceNumber;
        EntitySPtr entity;    
        bool entityDeleted = false, entityReportedHealth = false, componentReportedHealth = false;        
        {
            AcquireWriteLock lock(lock_);

            if(isClosed_)
            {
                // Object is closed
                return;
            }    

            auto entityIter = entityMap_.find(entityId);
            if(entityIter == entityMap_.end())
            {
                WriteNoise(        
                    TraceType,
                    Root.TraceId,
                    "UnregisterSource: Entity not found for EntityId:{0}. ComponentId:{1}",
                    entityId,
                    componentId);
                return;
            }    

            entity = entityIter->second;

            auto componentIter = entity->RegisteredComponents.find(componentId);
            if(componentIter == entity->RegisteredComponents.end())
            {
                WriteNoise(        
                    TraceType,
                    Root.TraceId,
                    "UnregisterSource: ComponentId:{0} is not registered. Entity:{1}",
                    componentId,
                    *entity);
                return;
            }

            componentReportedHealth = componentIter->second;

            entity->RegisteredComponents.erase(componentIter);            

            WriteNoise(        
                TraceType,
                Root.TraceId,
                "UnregisterSource: ComponentId:{0} is unregistered. Entity:{1}",
                componentId,
                *entity);

            if(entity->RegisteredComponents.empty())
            {
                entityMap_.erase(entityIter);                                
                entityDeleted = true;
                WriteInfo(        
                    TraceType,
                    Root.TraceId,
                    "UnregisterSource: Deleted healthy entity. Entity={0}",
                    *entity);
            }      

            entityReportedHealth = entity->HasSentReport;

            healthSequenceNumber = SequenceNumber::GetNext();
        }

        if(entityDeleted)
        {
            if(entityReportedHealth)
            {
                this->DeleteHealthEntity(
                    entity->EntityId,
                    entity->ApplicationName,
                    entity->InstanceId,
                    healthSequenceNumber);
            }
        }
        else if(componentReportedHealth)
        {
            this->DeleteHealthEvent(
                entity->EntityId,
                entity->ApplicationName, 
                entity->InstanceId, 
                componentId,
                healthSequenceNumber);
        }        
    }

    void ReportHealth(
        TEntity const & entityId,
        wstring const & componentId,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER sequenceNumber)
    {
        ASSERT_IF(componentId.empty(), "ComponentId cannot be empty. EntityId: {0}", entityId);

        EntitySPtr entity;    
        {
            AcquireReadLock lock(lock_);

            if(isClosed_) { return; }

            auto entityIter = entityMap_.find(entityId);
            if(entityIter == entityMap_.end())
            {
                WriteNoise(        
                    TraceType,
                    Root.TraceId,
                    "ReportHealth: Entity not found for EntityId:{0}. ComponentId:{1}",
                    entityId,
                    componentId);
                return;
            }

            entity = entityIter->second;

            auto componentIter = entity->RegisteredComponents.find(componentId);
            if(componentIter == entity->RegisteredComponents.end())
            {
                WriteNoise(        
                    TraceType,
                    Root.TraceId,
                    "ReportHealth: Component is not registered for the entity. EntityId:{0}, ComponentId:{1}",
                    entityId,
                    componentId);
                return;
            }

            componentIter->second = true;

            entity->HasSentReport = true;
        } 

        this->ReportHealthEvent(entityId, entity->ApplicationName, entity->InstanceId, componentId, reportCode, healthDescription, sequenceNumber);
    }

protected:                   
    virtual Common::ErrorCode OnOpen()
    {
        ASSERT_IF(!this->healthClient_, "HealthClient must be initialized before calling open.");
        {
            AcquireWriteLock lock(this->lock_);
            isClosed_ = false;
        }

        return ErrorCodeValue::Success;
    }

    virtual Common::ErrorCode OnClose()
    {
        map<TEntity, EntitySPtr> map;

        {
            AcquireWriteLock lock(this->lock_);
            map = entityMap_;
            entityMap_.clear();
            isClosed_ = true;
        }    

        for(auto iter = map.begin(); iter != map.end(); ++iter)
        {
            EntitySPtr entity = iter->second;
            this->DeleteHealthEntity(
                entity->EntityId,
                entity->ApplicationName,
                entity->InstanceId,
                SequenceNumber::GetNext());
        }

        return ErrorCodeValue::Success;
    }

    virtual void OnAbort()
    {
        this->OnClose().ReadValue();
    }

    virtual void ReportHealthEvent(
        TEntity const & entity, 
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber) = 0;

    virtual void DeleteHealthEvent(
        TEntity const & entity, 
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber) = 0;

    virtual void DeleteHealthEntity(
        TEntity const & entity, 
        wstring const & applicationName, 
        int64 const instanceId,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber) = 0;                

protected:
    HostingSubsystem & hosting_;
    HealthReportingComponentSPtr healthClient_;

private:
    struct Entity;
    typedef shared_ptr<Entity> EntitySPtr;    

private:    
    map<TEntity, EntitySPtr> entityMap_;
    Common::RwLock lock_;
    bool isClosed_;
};

template <class TEntity>
struct HostingHealthManager::EntityManagerBase<TEntity>::Entity
{       
    DENY_COPY(Entity)
public:

    Entity(TEntity const & entityId, wstring const & applicationName)
        : EntityId(entityId),
        ApplicationName(applicationName),
        InstanceId(SequenceNumber::GetNext()),
        RegisteredComponents(),
        HasSentReport(false)
    {
    }

    ~Entity() {}

    void WriteTo(TextWriter& w, FormatOptions const&) const
    {
        w.Write("{ ");
        w.Write("Id={0},", EntityId);
        w.Write("ApplicationName={0},", ApplicationName);
        w.Write("InstanceId={0},", InstanceId);
        w.Write("HasSentReport={0},", HasSentReport);

        w.Write("RegisteredComponents={");
        for(auto iter = RegisteredComponents.begin(); iter != RegisteredComponents.end(); ++iter)
        {
            w.Write("{0},", *iter);
        }
        w.Write("}");        

        w.Write("}");
    }

public:
    TEntity const EntityId;
    wstring const ApplicationName;
    int64 const InstanceId;
    map<wstring, bool> RegisteredComponents;
    bool HasSentReport;
};  

class HostingHealthManager::ApplicationEntityManager : public HostingHealthManager::EntityManagerBase<ApplicationIdentifier>
{
public:
    ApplicationEntityManager(
        Common::ComponentRoot const & root,
        __in HostingSubsystem & hosting)
        : EntityManagerBase(root, hosting)
    {
    }

    ~ApplicationEntityManager() {}

protected:
    virtual void ReportHealthEvent(
        ApplicationIdentifier const & applicationId, 
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {     
        UNREFERENCED_PARAMETER(applicationId);

        auto entityInfo = EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(
            applicationName,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);
        AttributeList attributeList;
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, hosting_.NodeName);
        attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(hosting_.NodeInstanceId));

        auto healthReport = HealthReport::CreateSystemHealthReport(
            reportCode,
            move(entityInfo),
            propertyName,
            healthDescription,
            healthSequenceNumber,
            move(attributeList));

        WriteInfo(
            TraceType,
            Root.TraceId,
            "Application ReportHealth: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));

    }

    virtual void DeleteHealthEvent(
        ApplicationIdentifier const & applicationId,
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {
        UNREFERENCED_PARAMETER(applicationId);

        auto entityInfo = EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(
            applicationName,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);
        
        auto healthReport = HealthReport::CreateSystemRemoveHealthReport(
            SystemHealthReportCode::Hosting_DeleteEvent,
            move(entityInfo),
            propertyName,
            healthSequenceNumber);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "Application DeleteEvent: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }

    virtual void DeleteHealthEntity(
        ApplicationIdentifier const & applicationId, 
        wstring const & applicationName, 
        int64 const instanceId,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {        
        UNREFERENCED_PARAMETER(applicationId);

        auto entityInfo = EntityHealthInformation::CreateDeployedApplicationEntityHealthInformation(
            applicationName,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);

        auto healthReport = HealthReport::CreateSystemDeleteEntityHealthReport(
            SystemHealthReportCode::Hosting_DeleteDeployedApplication,
            move(entityInfo),
            healthSequenceNumber);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "Application DeleteEntity: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }
};

class HostingHealthManager::ServicePackageInstanceEntityManager 
    : public HostingHealthManager::EntityManagerBase<ServicePackageInstanceIdentifier>
{
public:
    ServicePackageInstanceEntityManager(
        Common::ComponentRoot const & root,
        __in HostingSubsystem & hosting)
        : EntityManagerBase(root, hosting)
    {
    }

    ~ServicePackageInstanceEntityManager() {}

protected:
    virtual void ReportHealthEvent(
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        SystemHealthReportCode::Enum reportCode,
        wstring const & healthDescription,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {     
        auto entityInfo = EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(
            applicationName,
            servicePackageInstanceId.ServicePackageName,
            servicePackageInstanceId.PublicActivationId,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);

        AttributeList attributeList;
        attributeList.AddAttribute(*HealthAttributeNames::NodeName, hosting_.NodeName);
        attributeList.AddAttribute(*HealthAttributeNames::NodeInstanceId, wformatString(hosting_.NodeInstanceId));

        auto healthReport = HealthReport::CreateSystemHealthReport(
            reportCode,
            move(entityInfo),
            propertyName,
            healthDescription,
            healthSequenceNumber,
            move(attributeList));

        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage ReportHealth: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }

    virtual void DeleteHealthEvent(
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        wstring const & applicationName, 
        int64 const instanceId,            
        wstring const & propertyName,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {
        auto entityInfo = EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(
            applicationName,
            servicePackageInstanceId.ServicePackageName,
            servicePackageInstanceId.PublicActivationId,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);
        
        auto healthReport = HealthReport::CreateSystemRemoveHealthReport(
            SystemHealthReportCode::Hosting_DeleteEvent,
            move(entityInfo),
            propertyName,
            healthSequenceNumber);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage DeleteEvent: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }

    virtual void DeleteHealthEntity(
        ServicePackageInstanceIdentifier const & servicePackageInstanceId,
        wstring const & applicationName, 
        int64 const instanceId,
        FABRIC_SEQUENCE_NUMBER healthSequenceNumber)
    {
        auto entityInfo = EntityHealthInformation::CreateDeployedServicePackageEntityHealthInformation(
            applicationName,
            servicePackageInstanceId.ServicePackageName,
            servicePackageInstanceId.PublicActivationId,
            hosting_.NodeIdAsLargeInteger,
            hosting_.NodeName,
            instanceId);
        auto healthReport = HealthReport::CreateSystemDeleteEntityHealthReport(
            SystemHealthReportCode::Hosting_DeleteDeployedServicePackage,
            move(entityInfo),
            healthSequenceNumber);

        WriteInfo(
            TraceType,
            Root.TraceId,
            "ServicePackage DeleteEntity: {0}",
            healthReport);

        healthClient_->AddHealthReport(move(healthReport));
    }
};

HostingHealthManager::HostingHealthManager(
    ComponentRoot const & root,
    __in HostingSubsystem & hosting)
    : RootedObject(root),
    applicationEntityManager_(),
    servicePackageEntityManager_(),
    hosting_(hosting)
{
    nodeEntityManager_ = make_unique<NodeEntityManager>(root, hosting);
    applicationEntityManager_ = make_unique<ApplicationEntityManager>(root, hosting);
    servicePackageEntityManager_ = make_unique<ServicePackageInstanceEntityManager>(root, hosting);    
}

HostingHealthManager::~HostingHealthManager()
{
}

void HostingHealthManager::InitializeHealthReportingComponent(Client::HealthReportingComponentSPtr const & healthClient)
{
    healthClient_ = healthClient;
    nodeEntityManager_->InitializeHealthReportingComponent(healthClient);
    applicationEntityManager_->InitializeHealthReportingComponent(healthClient);
    servicePackageEntityManager_->InitializeHealthReportingComponent(healthClient);
}

ErrorCode HostingHealthManager::OnOpen()
{
    auto error = nodeEntityManager_->Open();
    if(!error.IsSuccess())
    {
        return error;
    }

    error = applicationEntityManager_->Open();
    if(!error.IsSuccess())
    {
        return error;
    }

    error = servicePackageEntityManager_->Open();

    return error;
}

ErrorCode HostingHealthManager::OnClose()
{
    auto error = nodeEntityManager_->Close();
    if(!error.IsSuccess())
    {
        return error;
    }

    error = applicationEntityManager_->Close();
    if(!error.IsSuccess())
    {
        return error;
    }

    error = servicePackageEntityManager_->Close();

    return error;
}

void HostingHealthManager::OnAbort()
{
    nodeEntityManager_->Abort();
    applicationEntityManager_->Abort();
    servicePackageEntityManager_->Abort();
}


/* Methods for reporting health on Node*/
ErrorCode HostingHealthManager::RegisterSource(wstring const & componentId)
{
    return nodeEntityManager_->RegisterSource(componentId);
}

void HostingHealthManager::UnregisterSource(wstring const & componentId)
{
    return nodeEntityManager_->UnregisterSource(componentId);
}

void HostingHealthManager::ReportHealth(
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    return nodeEntityManager_->ReportHealth(
        componentId,
        reportCode,
        healthDescription,
        sequenceNumber);
}

void HostingHealthManager::ReportHealthWithTTL(
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber,
    Common::TimeSpan const timeToLive)
{
    return nodeEntityManager_->ReportHealthWithTTL(
        componentId,
        reportCode,
        healthDescription,
        sequenceNumber,
        timeToLive);
}

/* Methods for reporting health on Application*/
ErrorCode HostingHealthManager::RegisterSource(ApplicationIdentifier const & applicationId, wstring applicationName, wstring const & componentId)
{
    return applicationEntityManager_->RegisterSource(applicationId, applicationName, componentId);
}

void HostingHealthManager::UnregisterSource(ApplicationIdentifier const & applicationId, wstring const & componentId)
{
    return applicationEntityManager_->UnregisterSource(applicationId, componentId);
}

void HostingHealthManager::ReportHealth(
    ApplicationIdentifier const & applicationId,
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    return applicationEntityManager_->ReportHealth(
        applicationId,
        componentId,
        reportCode,
        healthDescription,
        sequenceNumber);
}

/* Methods for reporting health on ServicePackage*/
ErrorCode HostingHealthManager::RegisterSource(ServicePackageInstanceIdentifier const & servicePackageInstanceId, wstring applicationName, wstring const & componentId)
{
    return servicePackageEntityManager_->RegisterSource(servicePackageInstanceId, applicationName, componentId);
}

void HostingHealthManager::UnregisterSource(ServicePackageInstanceIdentifier const & servicePackageInstanceId, wstring const & componentId)
{
    return servicePackageEntityManager_->UnregisterSource(servicePackageInstanceId, componentId);
}

void HostingHealthManager::ReportHealth(
    ServicePackageInstanceIdentifier const & servicePackageInstanceId,
    wstring const & componentId,
    SystemHealthReportCode::Enum reportCode,
    wstring const & healthDescription,
    FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    return servicePackageEntityManager_->ReportHealth(
        servicePackageInstanceId,
        componentId,
        reportCode,
        healthDescription,
        sequenceNumber);
}

void HostingHealthManager::ForwardHealthReportFromApplicationHostToHealthManager(
    Transport::MessageUPtr && messagePtr,
    Transport::IpcReceiverContextUPtr && ipcTransportContext)
{
    Transport::MessageUPtr msgPtr = move(messagePtr);

    // Ensure HM header is set
    msgPtr->Headers.Replace(Transport::ActorHeader(Transport::Actor::HM));

    MoveUPtr<Transport::IpcReceiverContext> contextMover(move(ipcTransportContext));

    AsyncOperationSPtr sendToHealthManager =
        healthClient_->HealthReportingTransportObj.BeginReport(
        move(msgPtr),
        Client::ClientConfig::GetConfig().HealthOperationTimeout,
        [this, contextMover](AsyncOperationSPtr const & operation) mutable
    {
        Transport::IpcReceiverContextUPtr context = contextMover.TakeUPtr();

        this->SendToHealthManagerCompletedCallback(operation, move(context));
    },
        hosting_.Root.CreateAsyncOperationRoot());
}

void HostingHealthManager::SendToHealthManagerCompletedCallback(AsyncOperationSPtr const & asyncOperation, Transport::IpcReceiverContextUPtr && context)
{
    Transport::MessageUPtr reply;

    ErrorCode errorCode = healthClient_->HealthReportingTransportObj.EndReport(asyncOperation, reply);
    errorCode.ReadValue();

    if (reply != nullptr)
    {
        // No error check needed as errors will be sent back to ApplicationHost as part of reply.
        context->Reply(move(reply));
    }
}

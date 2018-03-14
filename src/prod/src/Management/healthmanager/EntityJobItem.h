// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Abstract class that represents a job item which can be enqueued and scheduled
        // by the entity job queue manager
        class EntityJobItem
            : public IHealthJobItem
        {
            DENY_COPY(EntityJobItem);
            
        public:
            virtual ~EntityJobItem();
            
            virtual std::wstring const & get_JobId() const { return entity_->EntityIdString; }
            
            __declspec(property(get=get_Entity)) std::shared_ptr<HealthEntity> const & Entity;
            std::shared_ptr<HealthEntity> const & get_Entity() const { return entity_; }

        protected:
            explicit EntityJobItem(std::shared_ptr<HealthEntity> const & entity);
                                                
        private:            
            std::shared_ptr<HealthEntity> entity_; 
        };
    }
}

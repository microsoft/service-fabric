// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class EntityStateFlags
        {
        public:
            explicit EntityStateFlags(bool markForDeletion = false);

            EntityStateFlags(EntityStateFlags const & other) = default;
            EntityStateFlags & operator=(EntityStateFlags const & other) = default;

            EntityStateFlags(EntityStateFlags && other) = default;
            EntityStateFlags & operator=(EntityStateFlags && other) = default;

            ~EntityStateFlags();

            __declspec (property(get=get_Value)) int Value;
            int get_Value() const { return value_; }
                        
            __declspec (property(get=get_IsMarkedForDeletion)) bool IsMarkedForDeletion;
            bool get_IsMarkedForDeletion() const { return ((value_ & MarkedForDeletion) != 0); }

            __declspec (property(get=get_IsCleanedUp)) bool IsCleanedUp;
            bool get_IsCleanedUp() const { return ((value_ & CleanedUp) != 0); }

            __declspec (property(get = get_IsInvalidated)) bool IsInvalidated;
            bool get_IsInvalidated() const { return ((value_ & Invalidated) == Invalidated); }

            __declspec (property(get=get_HasSystemReport,put=set_HasSystemReport)) bool HasSystemReport; 
            bool get_HasSystemReport() const { return hasSystemReport_; } 
            void set_HasSystemReport(bool value) { hasSystemReport_ = value; }

            __declspec (property(get=get_ExpectSystemReports,put=set_ExpectSystemReports)) bool ExpectSystemReports; 
            bool get_ExpectSystemReports() const { return expectSystemReports_; } 
            void set_ExpectSystemReports(bool value);

            __declspec (property(get=get_IsStale)) bool IsStale; 
            bool get_IsStale() const { return isStale_; } 

            __declspec (property(get=get_HasSystemError)) bool HasSystemError;
            bool get_HasSystemError() const { return systemErrorCount_ > 0; }

            __declspec (property(get=get_SystemErrorCount)) int SystemErrorCount;
            int get_SystemErrorCount() const { return systemErrorCount_; }
        
            bool ShouldBeCleanedUp() const;
            bool IsRecentlyInvalidated() const;
            
            bool HasNoChanges(Common::TimeSpan const & period) const;

            void MarkForDeletion();
            void UpdateStateModifiedTime();
            void MarkAsCleanedUp();
            void MarkAsStale();

            void Reset();
            void PrepareCleanup();

            void UpdateSystemErrorCount(int diff);

            void ResetSystemInfo();
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            
            FABRIC_PRIMITIVE_FIELDS_02(value_, lastModifiedTime_);
            
        private:
            enum Enum
            {
                None = 0x00, 
                MarkedForDeletion = 0x02,
                CleanedUp = 0x04,
                Invalidated = 0x0C, // Contains bit for CleanedUp, so IsCleanedUp returns true
            };

            int value_;
            Common::DateTime lastModifiedTime_;

            // Not persisted

            // Used by child entities to indicate that their copy to parent attributes is stale
            bool isStale_; 

            // Shows whether a system report was received by the entity
            bool hasSystemReport_; 

            // Shows whether the entity expects system reports or is created only from user reports and children
            bool expectSystemReports_;

            // Shows the number of system errors that have been received by the entity
            int systemErrorCount_;
        };
    }
}

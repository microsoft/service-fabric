// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class AttributesStoreData : public Store::StoreData
        {
        public:
            AttributesStoreData();

            explicit AttributesStoreData(Store::ReplicaActivityId const & activityId);

            AttributesStoreData(AttributesStoreData const & other) = default;
            AttributesStoreData & operator = (AttributesStoreData const & other) = default;

            AttributesStoreData(AttributesStoreData && other) = default;
            AttributesStoreData & operator = (AttributesStoreData && other) = default;

            virtual ~AttributesStoreData();

            __declspec (property(get = get_InstanceId)) FABRIC_INSTANCE_ID InstanceId;
            virtual FABRIC_INSTANCE_ID get_InstanceId() const = 0;

            __declspec (property(get = get_UseInstance)) bool UseInstance;
            bool get_UseInstance() const { return InstanceId != Constants::UnusedInstanceValue; }

            __declspec (property(get = get_HasInvalidInstance)) bool HasInvalidInstance;
            virtual bool get_HasInvalidInstance() const { return InstanceId == FABRIC_INVALID_INSTANCE_ID; }

            __declspec (property(get = get_StateFlags)) EntityStateFlags const & StateFlags;
            EntityStateFlags const & get_StateFlags() const { return stateFlags_; }

            __declspec (property(get = get_IsMarkedForDeletion)) bool IsMarkedForDeletion;
            bool get_IsMarkedForDeletion() const { return stateFlags_.IsMarkedForDeletion; }

            __declspec (property(get = get_IsCleanedUp)) bool IsCleanedUp;
            bool get_IsCleanedUp() const { return stateFlags_.IsCleanedUp; }

            __declspec (property(get = get_IsInvalidated)) bool IsInvalidated;
            bool get_IsInvalidated() const { return stateFlags_.IsInvalidated; }

            __declspec (property(get = get_IsStale)) bool IsStale;
            bool get_IsStale() const { return stateFlags_.IsStale; }

            __declspec (property(get = get_HasSystemReport, put = set_HasSystemReport)) bool HasSystemReport;
            bool get_HasSystemReport() const { return stateFlags_.HasSystemReport; }
            void set_HasSystemReport(bool value) { stateFlags_.HasSystemReport = value; }

            __declspec (property(get = get_ExpectSystemReports, put = set_ExpectSystemReports)) bool ExpectSystemReports;
            bool get_ExpectSystemReports() const { return stateFlags_.ExpectSystemReports; }
            void set_ExpectSystemReports(bool value) { stateFlags_.ExpectSystemReports = value; }

            __declspec (property(get = get_HasSystemError)) bool HasSystemError;
            bool get_HasSystemError() const { return stateFlags_.HasSystemError; }

            __declspec (property(get = get_SystemErrorCount)) int SystemErrorCount;
            int get_SystemErrorCount() const { return stateFlags_.SystemErrorCount; }

            bool ShouldBeCleanedUp() const { return stateFlags_.ShouldBeCleanedUp(); }

            void MarkForDeletion() { stateFlags_.MarkForDeletion(); }
            void UpdateStateModifiedTime() { stateFlags_.UpdateStateModifiedTime(); }
            void MarkAsCleanedUp() { stateFlags_.MarkAsCleanedUp(); }
            void MarkAsStale() { stateFlags_.MarkAsStale(); }
            bool HasNoChanges(Common::TimeSpan const & period) const { return stateFlags_.HasNoChanges(period); }
            void UpdateSystemErrorCount(int diff) { stateFlags_.UpdateSystemErrorCount(diff); }
            void ResetSystemInfo() { stateFlags_.ResetSystemInfo(); }
            void ResetState() { stateFlags_.Reset(); }
            void PrepareCleanup() { stateFlags_.PrepareCleanup(); }
            bool IsRecentlyInvalidated() const { return stateFlags_.IsRecentlyInvalidated(); }

            virtual Common::ErrorCode TryAcceptHealthReport(
                ServiceModel::HealthReport const & healthReport,
                __inout bool & skipLsnCheck,
                __inout bool & replaceAttributesMetadata) const;

            bool ShouldReplaceAttributes(AttributesStoreData const & other) const;

            // Override by derived components to set any parameters that are not persisted to store
            // based on values of persisted parameters.
            virtual void OnCompleteLoadFromStore() {}

            //
            // Pure virtual methods to be implemented by derived classes
            //
            virtual void SetAttributes(ServiceModel::AttributeList const & attributeList) = 0;
            virtual ServiceModel::AttributeList GetEntityAttributes() const = 0;
            virtual Common::ErrorCode HasAttributeMatch(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & isMatch) const = 0;
            virtual Common::ErrorCode GetAttributeValue(
                std::wstring const & attributeName,
                __inout std::wstring & attributeValue) const = 0;
            virtual bool ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const = 0;

            // Create new attributes with values from the health report
            virtual AttributesStoreDataSPtr CreateNewAttributes(ReportRequestContext const & context) const = 0;

            // Create new attributes with values from existing attributes
            virtual AttributesStoreDataSPtr CreateNewAttributesFromCurrent() const = 0;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const = 0;
            virtual void WriteToEtw(uint16 contextSequenceId) const = 0;

        protected:
            EntityStateFlags stateFlags_;
        };


        #define DECLARE_ATTRIBUTES_COMMON_METHODS( ATTRIB_TYPE, ENTITY_ID ) \
        public: \
                void SetAttributes(ServiceModel::AttributeList const & attributeList) override; \
                ServiceModel::AttributeList GetEntityAttributes() const override; \
                Common::ErrorCode HasAttributeMatch(std::wstring const & attributeName, std::wstring const & attributeValue, __out bool & isMatch) const override; \
                Common::ErrorCode GetAttributeValue(std::wstring const & attributeName, __inout std::wstring & attributeValue) const override; \
                AttributesStoreDataSPtr CreateNewAttributes(ReportRequestContext const & context) const override { \
                    return std::make_shared<ATTRIB_TYPE>(this->EntityId, context); \
                } \
                AttributesStoreDataSPtr CreateNewAttributesFromCurrent() const override { \
                    return std::make_shared<ATTRIB_TYPE>(*this); \
                } \
                bool ShouldUpdateAttributeInfo(ServiceModel::AttributeList const & attributeList) const override; \
        

        #define START_ATTRIBUTES_FLAGS( )   \
        public: \
        class AttributeFlags { \
        public: \
            AttributeFlags() : value_(0) {} \
            __declspec (property(get=get_Value)) int Value; \
            int get_Value() const { return value_; } \
            FABRIC_PRIMITIVE_FIELDS_01(value_); \
        private: \
            static const int None = 0; \
            int value_; 
                    
        #define ATTRIBUTES_FLAGS_ENTRY( ATTRIB_NAME, FLAG_VALUE )   \
        public: \
            void Set##ATTRIB_NAME() { value_ |= ATTRIB_NAME; } \
            bool Is##ATTRIB_NAME##Set() const { return (value_ & ATTRIB_NAME) != 0; } \
        private: \
            static const int ATTRIB_NAME = FLAG_VALUE; 

        #define END_ATTRIBUTES_FLAGS( )   \
        }; \
            __declspec (property(get=get_AttributeSetFlags)) AttributeFlags const & AttributeSetFlags; \
            AttributeFlags const & get_AttributeSetFlags() const { return attributeFlags_; } \
        private: \
        AttributeFlags attributeFlags_;
        
    } // HealthManager namespace
} // Management namespace

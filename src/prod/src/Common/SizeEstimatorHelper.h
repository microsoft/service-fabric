// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    // Classes that are meant as base class.
    // They shouldn't initialize serialization overhead, as the size should never be estimated.
    // Define the estimate method and the dynamic estimation only.
#define BEGIN_BASE_DYNAMIC_SIZE_ESTIMATION() \
    virtual size_t EstimateSize() const; \
    virtual size_t EstimateDynamicSerializationSize() const \
    {\
    size_t size = 0; \


    // Fully defined classes define serialization overhead (Static per class type)
    // and dynamic serialization size
#define BEGIN_DYNAMIC_SIZE_ESTIMATION() \
    public:\
    static size_t SerializationOverheadEstimate; \
    static volatile bool IsSerializationOverheadInitialized; \
    virtual size_t EstimateSize() const; \
    virtual size_t EstimateDynamicSerializationSize() const \
    {\
    size_t size = 0; \


#define DYNAMIC_SIZE_ESTIMATION_CHAIN() \
    size += __super::EstimateDynamicSerializationSize(); \

#define DYNAMIC_SIZE_ESTIMATION_MEMBER(Member) \
    size += Common::SizeEstimatorHelper::EstimateDynamicSerializationSize(Member); \

#define DYNAMIC_ENUM_ESTIMATION_MEMBER(Member) \
    size += 1; \
    
#define END_DYNAMIC_SIZE_ESTIMATION() \
    return size; \
}\


#define INITIALIZE_SIZE_ESTIMATION( CLASS ) \
    size_t CLASS::SerializationOverheadEstimate(0); \
    volatile bool CLASS::IsSerializationOverheadInitialized(false); \
    size_t CLASS::EstimateSize() const { \
        if (!IsSerializationOverheadInitialized) { \
            CLASS temp; \
            auto error = Common::FabricSerializer::EstimateSize(temp, SerializationOverheadEstimate); \
            if (error.IsSuccess()) { IsSerializationOverheadInitialized = true; } \
        } \
        return SerializationOverheadEstimate + EstimateDynamicSerializationSize(); \
    } \


#define INITIALIZE_TEMPLATED_SIZE_ESTIMATION( CLASS ) \
    template<> size_t CLASS::SerializationOverheadEstimate(0); \
    template<> volatile bool CLASS::IsSerializationOverheadInitialized(false); \
    template<> size_t CLASS::EstimateSize() const {    \
        if (!IsSerializationOverheadInitialized) { \
            CLASS temp; \
            auto error = Common::FabricSerializer::EstimateSize(temp, SerializationOverheadEstimate); \
            if (error.IsSuccess()) { IsSerializationOverheadInitialized = true; } \
        } \
        return SerializationOverheadEstimate + CLASS::EstimateDynamicSerializationSize(); \
    } \


#define INITIALIZE_BASE_DYNAMIC_SIZE_ESTIMATION( CLASS ) \
    size_t CLASS::EstimateSize() const {  \
        Common::Assert::TestAssert("EstimateSize on base class shouldn't be called"); \
        return EstimateDynamicSerializationSize(); \
    } \


#define VALUE_TYPE_EVALUATE_SIZE( TYPE ) \
    static size_t EvaluateSize(TYPE) { return sizeof(TYPE) + 1; } \
    static size_t EvaluateDynamicSize(TYPE) { return 0; } \


#define COMPRESSED_TYPE_EVALUATE_SIZE( TYPE ) \
    static size_t EvaluateSize(TYPE) { return sizeof(TYPE) + 1; } \
    static size_t EvaluateDynamicSize(TYPE value) { return (static_cast<int>(value) == 0) ? 0 : sizeof(TYPE); } \
    

    class SizeEstimatorHelper
    {
    public:
        template <class T>
        static size_t EstimateDynamicSerializationSize(T const & value)
        {
            __if_exists(IMPLEMENTS(T, EstimateDynamicSerializationSize))
            {
                return value.EstimateDynamicSerializationSize();
            }

            __if_not_exists(IMPLEMENTS(T, EstimateDynamicSerializationSize))
            {
                return EvaluateDynamicSize(value);
            }
        }
        
        template <class T>
        static size_t EstimateSize(T const & value)
        {
            __if_exists(T::EstimateSize)
            {
                return value.EstimateSize();
            }

            __if_not_exists(T::EstimateSize)
            {
                return EvaluateSize(value);
            }
        }
                
        VALUE_TYPE_EVALUATE_SIZE(bool)
        VALUE_TYPE_EVALUATE_SIZE(byte)
            
        COMPRESSED_TYPE_EVALUATE_SIZE(double)
        COMPRESSED_TYPE_EVALUATE_SIZE(__int64)
        COMPRESSED_TYPE_EVALUATE_SIZE(unsigned __int64)

#if !defined(PLATFORM_UNIX)
        COMPRESSED_TYPE_EVALUATE_SIZE(LONG)
        COMPRESSED_TYPE_EVALUATE_SIZE(ULONG)
#endif

        COMPRESSED_TYPE_EVALUATE_SIZE(int)
        COMPRESSED_TYPE_EVALUATE_SIZE(unsigned int)

        static size_t EvaluateDynamicSize(Federation::NodeId const & value)
        {
            // NodeId is serialized as byte array
            UNREFERENCED_PARAMETER(value);
            return 0;
        }

        static size_t EvaluateDynamicSize(std::vector<byte> const & value)
        {
            return value.size();
        }

        static size_t EvaluateDynamicSize(LargeInteger const & value)
        {
            return EvaluateDynamicSize(value.Low) + EvaluateDynamicSize(value.High);
        }

        static size_t EvaluateDynamicSize(Guid const & value)
        {
            if (value == Guid::Empty())
            {
                return 0;
            }

            return sizeof(value);
        }

        static size_t EvaluateDynamicSize(DateTime const & value) 
        {
            if (value == DateTime::Zero)
            {
                return 0;
            }

            // DateTime is serialized with ticks (int64)
            // If is was initialized with 0, the value was compressed.
            // Add overhead to account for this.
            return sizeof(value); 
        }

        static size_t EvaluateDynamicSize(TimeSpan const & value)
        {
            if (value == TimeSpan::Zero)
            {
                return 0;
            }

            // TimeSpan is serialized with ticks (int64)
            // If is was initialized with 0, the value was compressed.
            // Add overhead to account for this.
            return sizeof(value);
        }
        
        static size_t EvaluateDynamicSize(std::wstring const & value)
        {
            if (value.empty())
            {
                return 0;
            }

            return (value.size() + 1) * sizeof(wchar_t) - 1;
        }

        template<class TEntry>
        static size_t EvaluateSize(TEntry const & value)
        {
            return EvaluateDynamicSize(value) + 1;
        }
        
        template<class TEntry>
        static size_t EvaluateDynamicSize(std::shared_ptr<TEntry> const & value)
        {
            if (!value)
            {
                return 0;
            }

            return EstimateSize(*value);
        }

        template<class TEntry>
        static size_t EvaluateDynamicSize(std::unique_ptr<TEntry> const & value)
        {
            if (!value)
            {
                return 0;
            }

            return EstimateSize(*value);
        }

        template <class TEntry>
        static size_t EvaluateDynamicSize(std::vector<TEntry> const & v)
        {
            size_t size = 0; 
            if (!v.empty())
            {
                for (auto it = v.begin(); it != v.end(); ++it)
                {
                    size += EstimateSize(*it);
                }

                size += 1;
            }

            return size;
        }

        template<class TEntry>
        static size_t EvaluateDynamicSize(std::shared_ptr<std::vector<TEntry>> const & value)
        {
            if (!value)
            {
                return 0;
            }

            return EvaluateDynamicSize(*value);
        }

        template<class TEntry>
        static size_t EvaluateDynamicSize(std::unique_ptr<std::vector<TEntry>> const & value)
        {
            if (!value)
            {
                return 0;
            }

            return EvaluateDynamicSize(*value);
        }

        template <class TKey, class TValue>
        static size_t EvaluateDynamicSize(std::map<TKey, TValue> const & m)
        {
            size_t size = 0;
            if (!m.empty())
            {
                // Map is serialized as vector<ArrayPair<Key,Value>>
                // First get the estimate of the serialization overhead

                static volatile bool IsPairSerializationOverheadInitialized = false;
                static size_t PairOverhead = 0;
                if (!IsPairSerializationOverheadInitialized)
                {
                    ArrayPair<TKey, TValue> temp;
                    auto error = FabricSerializer::EstimateSize(temp, PairOverhead);
                    if (error.IsSuccess()) 
                    { 
                        IsPairSerializationOverheadInitialized = true; 
                    }
                }

                for (std::pair<TKey, TValue> const & item : m)
                {
                    size += PairOverhead +
                        SizeEstimatorHelper::EstimateDynamicSerializationSize(item.first) +
                        SizeEstimatorHelper::EstimateDynamicSerializationSize(item.second);
                }

                size += 1;
            }

            return size;
        }
    };
}

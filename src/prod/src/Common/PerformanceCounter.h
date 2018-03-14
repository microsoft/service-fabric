// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Common
{
	typedef LONGLONG PerformanceCounterValue;
	typedef unsigned __int32 PerformanceCounterId;

#define BEGIN_COUNTER_SET_DEFINITION(id, name, description, instanceType) \
public: \
	static Common::PerformanceCounterSet & GetCounterSet(); \
	inline static Common::PerformanceCounterSetDefinition const & Initialize() \
	{ \
		Common::Guid counterSetId(id); \
		Common::PerformanceCounterSetDefinition counterSet(counterSetId, name, description, instanceType); \

#define COUNTER_DEFINITION(counterId, counterType, name, description, ...) \
	{ \
		Common::PerformanceCounterDefinition counter(counterId, counterType, name, description); \
		counterSet.AddCounterDefinition(std::move(counter)); \
	} \

#define COUNTER_DEFINITION_WITH_BASE(counterId, baseCounterId, counterType, name, description, ...) \
	{ \
		Common::PerformanceCounterDefinition counter(counterId, baseCounterId, counterType, name, description); \
		counterSet.AddCounterDefinition(std::move(counter)); \
	} \

#define END_COUNTER_SET_DEFINITION() \
		Common::PerformanceProviderDefinition::Singleton()->AddCounterSetDefinition(std::move(counterSet)); \
		return Common::PerformanceProviderDefinition::Singleton()->GetCounterSetDefinition(counterSetId); \
	} \

#define DECLARE_COUNTER_INSTANCE(member) Common::PerformanceCounterData & member;

#define DECLARE_AVERAGE_TIME_COUNTER_INSTANCE(member) Common::PerformanceCounterAverageTimerData member; 

#define BEGIN_COUNTER_SET_INSTANCE(className) \
private: \
	Common::PerformanceCounterSetInstanceSPtr instance_; \
public: \
	static inline std::shared_ptr<className> CreateInstance(std::wstring const & instanceName) \
	{ \
		Common::PerformanceCounterSetInstanceSPtr instance = className::GetCounterSet().CreateCounterSetInstance(instanceName); \
		return std::make_shared<className>(std::move(instance)); \
	} \
	className(Common::PerformanceCounterSetInstanceSPtr && instance) : \
		
#define DEFINE_COUNTER_INSTANCE(member, counterId) member(instance->GetCounter(counterId)),
#define END_COUNTER_SET_INSTANCE() \
	instance_(std::move(instance)) \
	{ \
	} \
	
#define INITIALIZE_COUNTER_SET(className) \
	namespace \
	{ \
		static std::shared_ptr<Common::PerformanceCounterSet> className##__CounterSet; \
		static INIT_ONCE initOnce_##className; \
		static BOOL CALLBACK InitOnce##className(PINIT_ONCE, PVOID, PVOID*) \
		{ \
			Common::PerformanceCounterSetDefinition const & definition = className::Initialize(); \
			className##__CounterSet = std::make_shared<Common::PerformanceCounterSet>( \
				Common::PerformanceProviderDefinition::Singleton()->Identifier, \
				definition.Identifier, \
				definition.InstanceType); \
			auto counters = definition.CounterDefinitions; \
			for (auto it = begin(counters); end(counters) != it; ++it) \
			{ \
				className##__CounterSet->AddCounter(it->second.Identifier, it->second.Type); \
			} \
			return TRUE; \
		} \
	} \
	Common::PerformanceCounterSet & className::GetCounterSet(void) \
    { \
		BOOL  bStatus = FALSE; \
		PVOID lpContext = NULL; \
		bStatus = ::InitOnceExecuteOnce( \
			&initOnce_##className, \
			InitOnce##className, \
			NULL, \
			&lpContext); \
		return *className##__CounterSet; \
    } \

}

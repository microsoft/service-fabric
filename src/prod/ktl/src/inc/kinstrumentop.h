/*++

    (c) 2016 by Microsoft Corp. All Rights Reserved.

    kinstrumentop.h

    Description:
      Kernel Template Library (KTL): Operation instrumentation framework

      Class that can be used by a component to track and report information
      that is used to provide instrumentation information about the
      operations that it performs. The class tracks things such as

          * Min, max and average time for an operation over a period
          * Min, max and average size for an operation over a period
          * Min, max and average time between operations over a period
      
      The component calls BeginOperation() right before the operation
      is started and then EndOperation() right when the operation is
      completed. KInstrumentedComponent will keep track of the
      operation times and sizes and provide an optional report to the
      KTL tracing stream.

      Very important note: KInstrumentedOperation is typically embeded
      in the users operation class while KInstrumentedComponent is
      typically created and held by the class for the component that performs
      the operation. The KInstrumented* classes do no synchronization or references
      counting but instead rely upon the classes that contain them to do
      that. As long as the containing classes do not have issues then
      these classes should not either; all methods are synchronous.

    History:
      alanwar          03-Nov-2017         Initial version.

--*/

#pragma once

class KInstrumentedComponent : public KShared<KInstrumentedComponent>, public KObject<KInstrumentedComponent>
{
    K_FORCE_SHARED(KInstrumentedComponent);
    
    public:
        static NTSTATUS Create(__out KInstrumentedComponent::SPtr& IC,
                               __in KAllocator& Allocator,
                               __in ULONG AllocationTag
            );
        
        static const ULONG DefaultReportFrequency = 60 * 1000;   // every minute
        
        NTSTATUS SetComponentName(
            __in KStringView& Component
            );
        
        NTSTATUS SetComponentName(
            __in KStringView& ComponentPrefix,
            __in GUID Guid
            );

        NTSTATUS SetComponentName(
            __in KStringView& ComponentPrefix,
            __in GUID Guid1,
            __in GUID Guid2
            );

        inline void SetReportFrequency(
            __in ULONGLONG ReportFrequency
        )
        {
            _ReportFrequency = ReportFrequency;
        }
        
        inline void Report()
        {
            KDbgPrintfInformational("%s: %d Operations, %lld Total Size, %lld Min Size, %lld Max Size, %lld Total ms, %lld Min ms, %lld Max ms, %lld Between Total ms, %lld Between Min ms, %lld Between Max ms",
                                    (PCHAR)(*_ComponentName),
                                    _OperationCount,
                                    _OperationTotalSize,
                                    _OperationMinSize,
                                    _OperationMaxSize,
                                    _OperationTotalTime,
                                    _OperationMinTime,
                                    _OperationMaxTime,
                                    _BetweenOperationTotalTime,
                                    _BetweenOperationMinTime,
                                    _BetweenOperationMaxTime                                                                        
                                   );
            _LastReportTime = KNt::GetTickCount64();
        }

        inline void StartSampling()
        {
            _SampleStartTime = KNt::GetTickCount64();           
        }

        inline void Reset()
        {
            _OperationTotalTime = 0;
            _OperationMinTime = MAXULONGLONG;
            _OperationMaxTime = 0;
            _OperationCount = 0;
            _OperationTotalSize = 0;
            _OperationMinSize = MAXULONGLONG;
            _OperationMaxSize = 0;
            _LastOperationTime = 0;
            _BetweenOperationTotalTime = 0;
            _BetweenOperationMinTime = MAXULONGLONG;
            _BetweenOperationMaxTime = 0;                                                       
        }
        
        inline void ResetSample()
        {
            //
            // Setting this to zero will indicate that sampling is
            // disabled. Do this so we can consistently zero the rest
            // of the counters
            //
            _SampleStartTime = 0;           
            Reset();
            _SampleStartTime = KNt::GetTickCount64();
        }

        inline void GetResults(
            __out ULONGLONG& SampleTime,
            __out ULONG& OperationCount,
            __out ULONGLONG& OperationMinSize,
            __out ULONGLONG& OperationMaxSize,
            __out LONGLONG& OperationTotalSize,
            __out LONGLONG& OperationTotalTime,
            __out ULONGLONG& OperationMinTime,
            __out ULONGLONG& OperationMaxTime,
            __out LONGLONG& BetweenOperationTotalTime,
            __out ULONGLONG& BetweenOperationMinTime,
            __out ULONGLONG& BetweenOperationMaxTime                               
        )
        {
            ULONGLONG now = KNt::GetTickCount64();
            SampleTime = now - _SampleStartTime;
            OperationCount = _OperationCount;
            OperationMinSize = _OperationMinSize;
            OperationMaxSize = _OperationMaxSize;
            OperationTotalSize = _OperationTotalSize;
            OperationTotalTime = _OperationTotalTime;
            OperationMinTime = _OperationMinTime;
            OperationMaxTime = _OperationMaxTime;
            BetweenOperationTotalTime = _BetweenOperationTotalTime;
            BetweenOperationMinTime = _BetweenOperationMinTime;
            BetweenOperationMaxTime = _BetweenOperationMaxTime;
        }
    
        __inline void AccountForOperationStart()
        {
            ULONGLONG now = KNt::GetTickCount64();
            ULONGLONG between = _LastOperationTime == 0 ? 0 : (now - _LastOperationTime);

            _LastOperationTime = now;
            
            InterlockedAdd64(&_BetweenOperationTotalTime, between);
            
            if (between < _BetweenOperationMinTime)
            {
                _BetweenOperationMinTime = between;
            }

            if (between > _BetweenOperationMaxTime)
            {
                _BetweenOperationMaxTime = between;
            }
        }
        
        inline void AccountForOperationEnd(
            __in ULONGLONG OperationTime,
            __in ULONGLONG OperationSize
        )
        {
            if (_SampleStartTime == 0)
            {
                return;
            }

            InterlockedIncrement(&_OperationCount);
            
            InterlockedAdd64(&_OperationTotalTime, OperationTime);
            
            if (OperationTime < _OperationMinTime)
            {
                _OperationMinTime = OperationTime;
            }

            if (OperationTime > _OperationMaxTime)
            {
                _OperationMaxTime = OperationTime;
            }
            
            InterlockedAdd64(&_OperationTotalSize, OperationSize);

            if (OperationSize < _OperationMinSize)
            {
                _OperationMinSize = OperationSize;
            }

            if (OperationSize > _OperationMaxSize)
            {
                _OperationMaxSize = OperationSize;
            }

            ULONGLONG now = KNt::GetTickCount64();
            if ((now - _LastReportTime) > _ReportFrequency)
            {
                Report();
                ResetSample();
            }
        }
        
    private:
        KStringA::SPtr _ComponentName;
        ULONGLONG _ReportFrequency;          // Use MAX_ULONGLONG to disable reporting
        
        ULONGLONG _LastReportTime;

        volatile ULONGLONG _SampleStartTime;
        volatile LONGLONG _OperationTotalTime;
        volatile ULONGLONG _OperationMinTime;
        volatile ULONGLONG _OperationMaxTime;

        volatile LONG _OperationCount;
        volatile LONGLONG _OperationTotalSize;
        volatile ULONGLONG _OperationMinSize;
        volatile ULONGLONG _OperationMaxSize;

        volatile LONGLONG _LastOperationTime;
        volatile LONGLONG _BetweenOperationTotalTime;
        volatile ULONGLONG _BetweenOperationMinTime;
        volatile ULONGLONG _BetweenOperationMaxTime;
};

class KInstrumentedOperation
{
    public:
        KInstrumentedOperation();
        KInstrumentedOperation(__in KInstrumentedComponent& InstrumentedComponent);
        ~KInstrumentedOperation();

        inline void SetInstrumentedComponent(
            __in KInstrumentedComponent& InstrumentedComponent
        )
        {
            _InstrumentedComponent = &InstrumentedComponent;
        }
        
        inline void BeginOperation(
            )
        {
            _StartTime = KNt::GetTickCount64();
            _InstrumentedComponent->AccountForOperationStart();
        }

        inline void EndOperation(
            __in ULONGLONG Size
            )
        {
            ULONGLONG now = KNt::GetTickCount64();
            ULONGLONG time = now - _StartTime;
            _InstrumentedComponent->AccountForOperationEnd(time, Size);
        }

    private:
        ULONGLONG _StartTime;
        KInstrumentedComponent::SPtr _InstrumentedComponent;        
};

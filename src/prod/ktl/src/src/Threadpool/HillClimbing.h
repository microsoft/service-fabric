#pragma once

#include "Complex.h"
#include "Random.h"

namespace KtlThreadpool{

    class ThreadpoolMgr;

    class HillClimbingConfig
    {
    public:
        static const DWORD WavePeriod = 4;
        static const DWORD TargetSignalToNoiseRatio = 300;
        static const DWORD ErrorSmoothingFactor = 1;
        static const DWORD WaveMagnitudeMultiplier = 100;
        static const DWORD MaxWaveMagnitude = 20;
        static const DWORD WaveHistorySize = 8;
        static const DWORD Bias = 15;
        static const DWORD MaxChangePerSecond = 4;
        static const DWORD MaxChangePerSample = 20;
        static const DWORD MaxSampleErrorPercent = 15;
        static const DWORD SampleIntervalLow = 10;
        static const DWORD SampleIntervalHigh = 200;
        static const DWORD GainExponent = 200;
    };

    class HillClimbing
    {
    public:
        const double pi = 3.141592653589793;

        enum HillClimbingStateTransition
        {
            Warmup,
            Initializing,
            RandomMove,
            ClimbingMove,
            ChangePoint,
            Stabilizing,
            Starvation,     //used by ThreadpoolMgr
            ThreadTimedOut, //used by ThreadpoolMgr
            Undefined,
        };

        void Initialize(ThreadpoolMgr *tpmgr);

        int Update(int currentThreadCount, double sampleDuration, int numCompletions, int* pNewSampleInterval);

        void ForceChange(int newThreadCount, HillClimbingStateTransition transition);

    private:
        int         m_wavePeriod;
        int         m_samplesToMeasure;
        double      m_targetThroughputRatio;
        double      m_targetSignalToNoiseRatio;
        double      m_maxChangePerSecond;
        double      m_maxChangePerSample;
        int         m_maxThreadWaveMagnitude;
        DWORD       m_sampleIntervalLow;
        double      m_threadMagnitudeMultiplier;
        DWORD       m_sampleIntervalHigh;
        double      m_throughputErrorSmoothingFactor;
        double      m_gainExponent;
        double      m_maxSampleError;

        double      m_currentControlSetting;
        LONGLONG    m_totalSamples;
        int         m_lastThreadCount;
        double      m_elapsedSinceLastChange;       //elapsed seconds since last thread count change
        double      m_completionsSinceLastChange;   //number of completions since last thread count change

        double      m_averageThroughputNoise;

        double      *m_samples;                     //Circular buffer of the last m_samplesToMeasure samples
        double      *m_threadCounts;                //Thread counts effective at each of m_samples

        unsigned int    m_currentSampleInterval;
        Random          m_randomIntervalGenerator;

        int         m_accumulatedCompletionCount;
        double      m_accumulatedSampleDuration;

        void ChangeThreadCount(int newThreadCount, HillClimbingStateTransition transition);

        void LogTransition(int threadCount, double throughput, HillClimbingStateTransition transition);

        Complex GetWaveComponent(double* samples, int sampleCount, double period);

    private:

        ThreadpoolMgr *m_threadpoolMgr;

    private:

        static const int HillClimbingLogCapacity = 1024;

        struct HillClimbingLogEntry
        {
            DWORD TickCount;
            HillClimbingStateTransition Transition;
            int NewControlSetting;
            int LastHistoryCount;
            float LastHistoryMean;
        };

        HillClimbingLogEntry HillClimbingLog[HillClimbingLogCapacity];
        int HillClimbingLogFirstIndex;
        int HillClimbingLogSize;
    };
}

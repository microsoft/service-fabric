// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    using namespace std;

    StringLiteral ComponentRootTraceType("ComponentRoot");

    //
    // ComponentRootReference
    //

    typedef std::function<void(uint64)> ReleaseCallback;

    class ComponentRootReference : public ComponentRoot
    {
    public:
        ComponentRootReference(
            ComponentRootSPtr && root,
            uint64 id,
            ReleaseCallback const & callback)
            : ComponentRoot(false) // enableReferenceTracking
            , root_(move(root))
            , id_(id)
            , callback_(callback)
            , timestamp_(DateTime::Now())
            , stack_()
        {
            SetTraceId(root_->TraceId);
            stack_.CaptureCurrentPosition();
        }

        virtual ~ComponentRootReference()
        {
            callback_(id_);
            callback_ = 0;
        }

        void TraceTrackedReferences() const override
        {
            root_->TraceTrackedReferences();
        }

        void WriteTo(TextWriter& w, FormatOptions const&) const override
        {
            w.Write("[id={0}, utc={1}] stack='{2}'", 
                id_, 
                timestamp_,
                stack_);
        }

    private:
        ComponentRootSPtr root_;
        uint64 id_;
        ReleaseCallback callback_;
        DateTime timestamp_;
        StackTrace stack_;
    };

    //
    // ReferenceTracker
    //

    class ComponentRoot::ReferenceTracker
    {
    public:

        ReferenceTracker()
            : references_()
            , nextId_(0)
            , lock_()
        {
        }

        ComponentRootSPtr AddReference(ComponentRootSPtr && root)
        {
            auto id = ++nextId_;
            auto referenceSPtr = make_shared<ComponentRootReference>(
                move(root),
                id,
                [this](uint64 id) { this->OnReleaseReference(id); });

            {
                AcquireWriteLock lock(lock_);

                ComponentRootWPtr referenceWPtr = referenceSPtr;

                references_[id] = move(referenceWPtr);
            }

            return referenceSPtr;
        }

        void WriteTo(TextWriter& w, FormatOptions const&) const
        {
            // Keep strong references for tracing outside the 
            // lock since these may the last references and when 
            // they release, the OnReleaseReference() callback will 
            // be executed, acquiring the lock again.
            //
            unordered_map<uint64, ComponentRootSPtr> referencesCopy;
            {
                AcquireReadLock lock(lock_);

                for (auto const & reference : references_)
                {
                    auto sptr = reference.second.lock();

                    if (sptr.get() != nullptr)
                    {
                        referencesCopy[reference.first] = sptr;
                    }
                }
            }
                
            w.Write("count={0}\n", referencesCopy.size());

            for (auto const & reference : referencesCopy)
            {
                w.Write(*(reference.second));
            }
        }

    private:

        void OnReleaseReference(uint64 id)
        {
            AcquireWriteLock lock(lock_);

            references_.erase(id);
        }

        unordered_map<uint64, ComponentRootWPtr> references_;
        atomic_uint64 nextId_;
        mutable RwLock lock_;
    };

    //
    // ComponentRoot
    //

    ComponentRoot::ComponentRoot(bool enableReferenceTracking)
        : enable_shared_from_this()
        , traceId_()
        , referenceTracker_()
    {
        if (enableReferenceTracking)
        {
            referenceTracker_ = make_unique<ReferenceTracker>();
        }
    }

    ComponentRoot::ComponentRoot(ComponentRoot const & other)
        : enable_shared_from_this(other)
        , traceId_(other.traceId_)
        , referenceTracker_()
        , leakDetectionTimer_()
    {
    }

    ComponentRoot::ComponentRoot(ComponentRoot && other)
        : enable_shared_from_this(other)
        , traceId_(move(other.traceId_))
        , referenceTracker_()
        , leakDetectionTimer_()
    {
    }

    ComponentRoot::~ComponentRoot()
    {
        if (leakDetectionTimer_.get() != nullptr)
        {
            Trace.WriteInfo(ComponentRootTraceType, "{0} Cancelling leak detection timer", this->TraceId);
            
            leakDetectionTimer_->Cancel();
        }
    }

    ComponentRootSPtr ComponentRoot::CreateComponentRoot() const
    {
        return (referenceTracker_.get() == nullptr)
            ? shared_from_this()
            : referenceTracker_->AddReference(shared_from_this());
    }

    AsyncOperationSPtr ComponentRoot::CreateAsyncOperationRoot() const
    {
        return AsyncOperationRoot<ComponentRootSPtr>::Create(this->CreateComponentRoot());
    }

    AsyncOperationSPtr ComponentRoot::CreateAsyncOperationMultiRoot(std::vector<ComponentRootSPtr> && roots)
    {
        return AsyncOperationRoot<std::vector<ComponentRootSPtr>>::Create(std::move(roots));
    }

    void ComponentRoot::TraceTrackedReferences() const
    {
        if (leakDetectionTimer_.get() != nullptr)
        {
            leakDetectionTimer_->Cancel();
        }

        if (referenceTracker_.get() == nullptr)
        {
            Trace.WriteWarning(ComponentRootTraceType, "{0} TraceTrackedReferences: Reference tracking is disabled", this->TraceId);
        }
        else
        {
            Trace.WriteError(ComponentRootTraceType, "{0} Outstanding references: {1}", this->TraceId, *referenceTracker_);
        }
    }

    void ComponentRoot::TryStartLeakDetectionTimer()
    {
        this->TryStartLeakDetectionTimer(CommonConfig::GetConfig().LeakDetectionTimeout);
    }

    void ComponentRoot::TryStartLeakDetectionTimer(TimeSpan const timeout)
    {
        if (referenceTracker_.get() == nullptr)
        {
            Trace.WriteInfo(ComponentRootTraceType, "{0} TryStartLeakDetectionTimer: Reference tracking is disabled", this->TraceId);
        }
        else if (leakDetectionTimer_.get() == nullptr)
        {
            ComponentRootWPtr weakRoot = this->CreateComponentRoot();
            leakDetectionTimer_ = Timer::Create(ComponentRootTraceType, [weakRoot] (TimerSPtr const &) { LeakDetectionTimerCallback(weakRoot); });

            Trace.WriteInfo(ComponentRootTraceType, "{0} Scheduling leak detection timer: {1}", this->TraceId, timeout);
            leakDetectionTimer_->Change(timeout);
        }
    }

    void ComponentRoot::LeakDetectionTimerCallback(ComponentRootWPtr const & thisWPtr)
    {
        auto thisSPtr = thisWPtr.lock();
        if (thisSPtr.get() != nullptr)
        {
            Trace.WriteWarning(ComponentRootTraceType, "{0} Possible leak detected", thisSPtr->TraceId);

            thisSPtr->TraceTrackedReferences();
        }
    }

    void ComponentRoot::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << L"Root[" << traceId_ << L']';
    }

    void ComponentRoot::SetTraceId(std::wstring const & traceId, size_t capacity)
    { 
        traceId_ = traceId; 

        if (capacity > 0)
        {
            traceId_.reserve(capacity);
        }
    }

    bool ComponentRoot::TryPutReservedTraceId(std::wstring const & traceId) 
    { 
        if (traceId.size() <= traceId_.capacity())
        {
            traceId_ = traceId; 
            return true;
        }
        else
        {
            return false;
        }
    }
}

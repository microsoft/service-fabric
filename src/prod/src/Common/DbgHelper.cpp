// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <execinfo.h>
#include <cxxabi.h>

using namespace Common;
using namespace std;

static const StringLiteral TraceType("BackTrace");

string DbgHelper::CxaDemangle(const char* mangledName)
{
    int status;
    size_t length = 0;
    char* demangled = abi::__cxa_demangle(mangledName, 0, &length, &status);
    KFinally([=] { if (demangled) free(demangled); });

    if (status == 0)
    {
        return string(demangled);
    }

    return mangledName;
}

map<void*, string> DbgHelper::Addr2Line(string const & module, vector<void*> const & addresses)
{
//    Trace.WriteNoise(TraceType, "addr2line -e {0}, address count = {1}", module, addresses.size());

    string cmdline;
    cmdline.reserve(module.size() + (2*sizeof(void*) + 1)*addresses.size() + 32);
    cmdline.append(formatString("addr2line -e {0}", module));
    for (auto const & addr : addresses)
    {
        cmdline.append(formatString(" {0}", addr));
    }

    map<void*, string> results;
    auto outf = popen(cmdline.c_str(), "r");
    KFinally([&]
    {
        if (outf && (pclose(outf) < 0))
        {
            results.clear(); //addr2line failed
        }
    });

    char buf[1024];
    int i = 0;
    for(; ; ++i)
    {
        char *line = buf;
        size_t len = sizeof(buf);
        auto read = getline(&line, &len, outf);
        if (read <= 0) break;

        if (buf[read-1] == '\n') buf[read-1] = 0;

        if (buf[0] && (buf[0] != '?'))
        {
            //!!! It is assumed that addr2line output ordering is the same as input addresses
            results.emplace(addresses[i], buf);
        }
    }

    return results;
}

BackTrace::BackTrace(void* const * rawBacktrace, size_t frameCount)
{
    Initialize(rawBacktrace, frameCount);
}

BackTrace::BackTrace(int frameToCapture, int skipCount)
{
//    Trace.WriteNoise(TraceType, "frameToCapture = {0}, skipCount = {1}", frameToCapture, skipCount);
    Invariant(frameToCapture > 0);
    Invariant(skipCount>=0);

    auto totalToCapture = frameToCapture + skipCount + 1;
    vector<void*> bt(totalToCapture, 0);
    auto captured = backtrace(bt.data(), totalToCapture);
    Invariant(captured > 0);
    if (bt[captured-1] == 0)
    {
       --captured; //remove trailing 0, backtrace() bug? 
    }

    bt.resize(captured);
//    Trace.WriteNoise(TraceType, "totalToCapture = {0}, captured = {1}", totalToCapture, captured); 

    Initialize(&(bt[skipCount + 1]), std::max(captured - skipCount - 1, 0));
}

void BackTrace::Initialize(void* const * bt, size_t count)
{
//    Trace.WriteNoise(TraceType, "Initialize({0}, {1})", TextTracePtr(bt), count);
    backtrace_.resize(count);
    memcpy(backtrace_.data(), bt, count * sizeof(void*));

    moduleList_.resize(count);
    funcAddrList_.resize(count);
    srcFileLineNoList_.resize(count);
}

ErrorCode BackTrace::ResolveSymbols()
{
    auto count = backtrace_.size();
    auto symbols = backtrace_symbols(backtrace_.data(), count);
    if (!symbols)
    {
        auto errNo = errno;
        Trace.WriteError(TraceType, "backtrace_symbols failed: {0}", errNo);

        for(int i = 0; i < backtrace_.size(); ++i)
        {
            moduleList_[i] = "???";
            funcAddrList_[i] = formatString("{0:x}", backtrace_[i]);
        }

        return ErrorCode::FromErrno(errNo);
    }

    KFinally([=] { free(symbols); }); 

    map<string, vector<void*>> moduleMap; // group addresses by module
    for(int i = 0; i < count; ++i)
    {
        auto symbol = symbols[i];
//        Trace.WriteNoise(TraceType, "raw: {0}", symbol); 

        // find parentheses and +address offset surrounding the mangled function name, e.g.
        // module(function+0x15c) [0x8048a6d]
        char *beginName = 0;
        char *beginOffset = 0;
        char *endOffset = 0;
        for (char *p = symbol; *p; ++p)
        {
            if (*p == '(')
                beginName = p;
            else if (*p == '+')
                beginOffset = p;
            else if (*p == ')' && beginOffset)
            {
                endOffset = p;
                break;
            }
        }

        if (!beginName)
        {
            moduleList_[i] = string(symbol);
            continue;
        }

        *beginName = '\0';
        moduleList_[i] = symbol;
        string const & module = moduleList_[i];
//        Trace.WriteNoise(TraceType, "module: {0}", module); 
        auto moduleIter = moduleMap.find(module);
        if (moduleIter == moduleMap.cend())
        {
            moduleIter = moduleMap.emplace(module, vector<void*>()).first;
            moduleIter->second.reserve(count);
        }

        moduleIter->second.emplace_back(backtrace_[i]);

        if (!beginOffset || !endOffset || beginName >= beginOffset)
        {
            funcAddrList_[i] = formatString("[0x{0}]", backtrace_[i]); 
            continue;
        }

        beginName++;
        *beginOffset++ = '\0';
        *endOffset = '\0';
        
//        Trace.WriteNoise(TraceType, "mangled name: {0}", beginName); 
        funcAddrList_[i] = formatString("{0}+{1}", DbgHelper::CxaDemangle(beginName), beginOffset);
    }

    for(auto const & m : moduleMap)
    {
        auto fileLineNoMap = DbgHelper::Addr2Line(m.first, m.second);
        for(int i = 0; i < srcFileLineNoList_.size(); ++i)
        {
            auto iter = fileLineNoMap.find(backtrace_[i]);
            if (iter == fileLineNoMap.cend()) continue;

            srcFileLineNoList_[i] = iter->second;
        }
    }

    return ErrorCodeValue::Success;
}

bool BackTrace::operator == (BackTrace const & rhs) const
{
    return
        (backtrace_.size() == rhs.backtrace_.size()) &&
        (memcmp(backtrace_.data(), rhs.backtrace_.data(), backtrace_.size() * sizeof(void*)) == 0);
}

bool BackTrace::operator != (BackTrace const & rhs) const
{
    return !(*this == rhs);
}

bool BackTrace::operator < (BackTrace const & rhs) const
{
    if (backtrace_.size() < rhs.backtrace_.size()) return true;

    if (backtrace_.size() > rhs.backtrace_.size()) return false;

    // size == rhs.size
    return memcmp(backtrace_.data(), rhs.backtrace_.data(), backtrace_.size() * sizeof(void*)) < 0;
}

void BackTrace::WriteTo(TextWriter &w, FormatOptions const&) const
{
    auto frameCount = moduleList_.size();
    Invariant(frameCount == funcAddrList_.size());
    Invariant(frameCount == srcFileLineNoList_.size());

    FormatOptions fo(2, true, ""); 
    for (int i = 0; i < frameCount; i++)
    {
        w.WriteNumber(i, fo, false); 
        w.WriteLine("  {0} : {1}", moduleList_[i], funcAddrList_[i]);
        if (!srcFileLineNoList_[i].empty())
        {
            w.WriteLine("    [{0}]", srcFileLineNoList_[i]);
        }
    }
}

CallStackRanking::CallStackRanking(int frameToCapture) : frameToCapture_(frameToCapture), symbolResolved_(false)
{
}

void CallStackRanking::Track()
{
    BackTrace bt(frameToCapture_, 1); 

    AcquireWriteLock grab(lock_);

    auto iter = callstacks_.find(bt);
    if (iter == callstacks_.cend())
    {
        callstacks_.emplace(move(bt), 1);
        return;
    }

    ++(iter->second);
}

void CallStackRanking::ResolveSymbols()
{
    AcquireWriteLock grab(lock_);

    if (symbolResolved_) return;

    symbolResolved_ = true;

    for(auto & e : callstacks_)
    {
        // const_cast is needed here because map does not allow changing key in-place.
        // It is okay to call ResolveSymbols(), as it does not affect actual key value.
        (const_cast<BackTrace&>(e.first)).ResolveSymbols();
    }
}

void CallStackRanking::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    multimap<size_t, BackTrace> ranking;
    {
        AcquireReadLock grab(lock_);

        Invariant(symbolResolved_);

        for(auto const & e : callstacks_)
        {
            ranking.emplace(e.second, e.first);
        }
    }

    for(auto & e : ranking)
    {
        w.WriteLine("<< count = {0} >>", e.first);
        w.WriteLine("{0}", e.second);
    }
}

map<BackTrace, size_t> CallStackRanking::Callstacks() const
{
    AcquireReadLock grab(lock_);
    return callstacks_;
}

// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
    TraceConsoleSink* TraceConsoleSink::Singleton = new TraceConsoleSink();

    TraceConsoleSink::TraceConsoleSink()
        :   console_(),
            color_(console_.Color())
    {
    }

    void TraceConsoleSink::Write(LogLevel::Enum level, std::wstring const & data)
    {
        AcquireWriteLock lock(Singleton->lock_);
        {
            if (level == LogLevel::Error || level == LogLevel::Critical)
            {
                Singleton->PrivateWrite(0x0c, data);
            }
            else if (level == LogLevel::Warning)
            {
                Singleton->PrivateWrite(0x0e, data);
            }
            else
            {
                Singleton->PrivateWrite(data);
            }
        }
    }
  
    void TraceConsoleSink::Write(DWORD color, std::wstring const & data)
    {
        AcquireWriteLock lock(Singleton->lock_);
        {
            Singleton->PrivateWrite(color, data);
        }
    }

    void TraceConsoleSink::PrivateWrite(std::wstring const & data)
    {
        console_.WriteUnicodeBuffer(data.c_str(), data.size());
        wstring newLine = L"\r\n";
        console_.WriteUnicodeBuffer(newLine.c_str(), newLine.size());
        console_.Flush();
    }

    void TraceConsoleSink::PrivateWrite(DWORD color, std::wstring const & data)
    {
        console_.SetColor(color);
        PrivateWrite(data);
        console_.SetColor(color_);
    }
}

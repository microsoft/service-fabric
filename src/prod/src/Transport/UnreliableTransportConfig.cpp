// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <fstream>

using namespace Transport;
using namespace std;
using namespace Common;

GlobalWString UnreliableTransportConfig::SettingsFileName = make_global<std::wstring>(L"UnreliableTransportSettings.ini");
static const StringLiteral TraceType("Config");

void UnreliableTransportConfig::Initialize()
{
    isDisabledUnreliableTransport_ = true;
    LoadConfiguration(config_);
}

TimeSpan UnreliableTransportConfig::GetDelay(std::wstring const & source, std::wstring const & destination, std::wstring const & action)
{
    AcquireReadLock lock(specsTableLock_);

    int priority = -1;
    TimeSpan delay = TimeSpan::Zero;

    for (auto it = specifications_.begin(); it != specifications_.end(); ++it)
    {
        if ((*it)->GetPriority() > priority && (*it)->Match(source, destination, action))
        {
            priority = (*it)->GetPriority();
            delay = (*it)->GetDelay();
        }
    }

    return delay > MaxAllowedDelayInSeconds ? TimeSpan::MaxValue : delay;
}

TimeSpan UnreliableTransportConfig::GetDelay(std::wstring const & source, std::wstring const & destination, Transport::Message const & message)
{
    AcquireReadLock lock(specsTableLock_);

    int priority = -1;
    TimeSpan delay = TimeSpan::Zero;

    for (auto it = specifications_.begin(); it != specifications_.end(); ++it)
    {
        if ((*it)->GetPriority() > priority && (*it)->Match(source, destination, message))
        {
            priority = (*it)->GetPriority();
            delay = (*it)->GetDelay();
        }
    }

    return delay > MaxAllowedDelayInSeconds ? TimeSpan::MaxValue : delay;
}

TimeSpan ParseTimeSpan(wstring const & data)
{
    if (StringUtility::AreEqualCaseInsensitive(data, L"Max"))
    {
        return TimeSpan::MaxValue;
    }

    return TimeSpan::FromSeconds(Common::Double_Parse(data));
}

ErrorCode UnreliableTransportConfig::StartMonitoring(wstring const & workFolder, wstring const & nodeId)
{
    AcquireExclusiveLock lock(fileChangeMonitorDictionaryLock_);

    // checking if a file monitor does not exist for this nodeid in the dictionary
    if (nodeIdFileChangeMonitor.count(nodeId) == 0 && workFolder != L"")
    {
        // constructing path to file to be monitored        
        wstring fileName = Path::Combine(workFolder, UnreliableTransportConfig::SettingsFileName);
        Path::MakeAbsolute(fileName);
        UnreliableTransport::WriteInfo(TraceType, "Creating File Monitor at {0} for nodeid: {1}", fileName, nodeId);

        // If the file does not exist it creates an empty one        
        {
            FileWriterLock fileLock(fileName);
            if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
            {
                UnreliableTransport::WriteWarning(
                    TraceType,
                    "Failed to acquire UnreliableTransportSettings lock file to start monitoring. Associated file = {0}",
                    fileName);
                return ErrorCode(ErrorCodeValue::Timeout);
            }

            if (!File::Exists(fileName))
            {
                // creating file
                Common::File file;
                auto error = file.TryOpen(fileName, FileMode::OpenOrCreate, FileAccess::Write);
                if (!error.IsSuccess())
                {
                    UnreliableTransport::WriteWarning(
                        TraceType,
                        "Failed to create Unreliable Transport Configuration file at {0}: {1}", fileName, error);
                    return error;
                }
            }

            // loading existing configurations
            LoadConfigurationFromINI(fileName, nodeId, false /* it is already locked here */);

            // registering callback for monitoring Unreliable Transport Configuration changes            
            FileChangeMonitor2SPtr fileMonitor = FileChangeMonitor2::Create(fileName);
            fileMonitor->Open([this, fileName, nodeId](FileChangeMonitor2SPtr const & monitor) {UNREFERENCED_PARAMETER(monitor); LoadConfigurationFromINI(fileName, nodeId); });
            nodeIdFileChangeMonitor[nodeId] = fileMonitor;
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}


// this method is called only in tests where multiple nodes are hosted in the same process and share the same UnreliableTransportConfig object
// allows cleaning up file monitors monitoring files that you be deleted.
ErrorCode UnreliableTransportConfig::Test_StopMonitoring(wstring const & nodeId)
{
    AcquireExclusiveLock lock(fileChangeMonitorDictionaryLock_);

    // checking if a file monitor does not exist for this nodeid in the dictionary
    auto it = nodeIdFileChangeMonitor.find(nodeId);
    if (it != nodeIdFileChangeMonitor.end())
    {
        it->second->Close();
        nodeIdFileChangeMonitor.erase(it);
        UnreliableTransport::WriteInfo(TraceType, "Removing File Monitor for nodeid: {0}", nodeId);
    } 

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode UnreliableTransportConfig::ReadFromINI(std::wstring const & fileName, vector<wstring> & parameters, bool shouldLockFile)
{
    string configText;
    ErrorCode retReadFile;

    // acquiring lock and reading from file
    if (shouldLockFile)
    {
        FileReaderLock fileLock(fileName);
        if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
        {
            UnreliableTransport::WriteWarning(
                TraceType,
                "Failed to acquire UnreliableTransportSettings lock file for reading. Associated file = {0}",
                fileName);
            return ErrorCode(ErrorCodeValue::Timeout);
        }
        retReadFile = ReadTextFile(fileName, configText);
    }
    else
    {
        retReadFile = ReadTextFile(fileName, configText);
    }

    if (!retReadFile.IsSuccess())
    {
        return retReadFile;
    }

    istringstream configStream(configText);

    string line;
    // reads line by line of UnreliableTransportSettings.ini formated file
    while (std::getline(configStream, line)){
        wstring wname(line.begin(), line.end());
        parameters.push_back(wname);
    }

    return ErrorCodeValue::Success;
}

ErrorCode UnreliableTransportConfig::ReadFromINI(std::wstring const & fileName, vector<pair<wstring, wstring> > & parameters, bool shouldLockFile)
{
    string configText;
    ErrorCode retReadFile;

    // acquiring lock and reading from file
    if (shouldLockFile)
    {
        FileReaderLock fileLock(fileName);
        if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
        {
            UnreliableTransport::WriteWarning(
                TraceType,
                "Failed to acquire UnreliableTransportSettings lock file for reading. Associated file = {0}",
                fileName);
            return ErrorCode(ErrorCodeValue::Timeout);
        }
        retReadFile = ReadTextFile(fileName, configText);
    }
    else
    {
        retReadFile = ReadTextFile(fileName, configText);
    }

    if (!retReadFile.IsSuccess())
    {
        return retReadFile;
    }

    istringstream configStream(configText);

    string line;
    // reads line by line of UnreliableTransportSettings.ini formated file
    while (std::getline(configStream, line)){
        istringstream lineStream(line);
        string name;
        string value;

        // breaks into name (separated by = character), and parameters
        std::getline(lineStream, name, '=');
        std::getline(lineStream, value);

        wstring wname(name.begin(), name.end());
        wstring wvalue(value.begin(), value.end());

        // removing \r from end of string
        if (wvalue.size() && wvalue.at(wvalue.size() - 1) == L'\r')
        {
            wvalue = wstring(wvalue.begin(), --wvalue.end());
        }

        parameters.push_back(make_pair(wname, wvalue));
    }

    return ErrorCodeValue::Success;
}

ErrorCode UnreliableTransportConfig::WriteToINI(std::wstring const & fileName, vector<pair<wstring, wstring> > const & parameters, bool shouldLockFile)
{
    ErrorCode retWriteFile;

    // acquiring lock and reading from file
    if (shouldLockFile)
    {
        FileWriterLock fileLock(fileName);
        if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
        {
            UnreliableTransport::WriteWarning(
                TraceType,
                "Failed to acquire UnreliableTransportSettings Lock file for writing. Associated file = {0}",
                fileName);
            return ErrorCode(ErrorCodeValue::Timeout);
        }
        retWriteFile = WriteTextFile(fileName, parameters);
    }
    else
    {
        retWriteFile = WriteTextFile(fileName, parameters);
    }
       
    // File notifications may come in faster than FileChangeMonitor's differentiating logic.  8638641 is tracking the bigger fix.
    Sleep(10);
       
    return retWriteFile;
}

ErrorCode UnreliableTransportConfig::ReadTextFile(wstring const & fileName, string & text)
{
    File fileReader;

    auto error = fileReader.TryOpen(fileName, FileMode::Open, FileAccess::Read, FileShare::Read);
    if (!error.IsSuccess())
    {
        UnreliableTransport::WriteError(
            TraceType,
            "Failed to open UnreliableTransportSettings file for reading at {0}: {1}",
            fileName,
            error);
        return ErrorCodeValue::UnspecifiedError;
    }
    // TODO: if files are large, read line by line
    // instead of all file content
    int64 fileSize = fileReader.size();
    // For now, we don't expect the files to be large.
    ASSERT_IF(fileSize > 100000, "File size is {0}", fileSize);
    size_t size = static_cast<size_t>(fileSize);
    // TODO: use streams instead of reading bytes,
    // because they know how to deal with Unicode/ASCII
    // For now, until the build team fully integrates VC10, 
    // we do not have access to locale, regex and stream parts of the VC10 standard library        
    text.resize(size);
    fileReader.Read(&text[0], static_cast<int>(size));
    fileReader.Close();

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode UnreliableTransportConfig::WriteTextFile(wstring const & fileName, std::vector<pair<wstring, wstring> > const & parameters)
{
    File file;
    auto error = file.TryOpen(
        fileName,
        FileMode::Create,
        FileAccess::Write);

    if (!error.IsSuccess())
    {
        UnreliableTransport::WriteError(
            TraceType,
            "Failed to open UnreliableTransportSettings file '{0}' for writing: {1}",
            fileName,
            error);

        return ErrorCode(ErrorCodeValue::UnspecifiedError);
    }

    UnreliableTransport::WriteInfo(TraceType, "Updating unreliable transport config");

    string s;
    StringWriterA sw(s);

    for (auto parameter : parameters)
    {
        sw.Write("{0}={1}\n", parameter.first, parameter.second);
    }

    auto written = file.TryWrite(s.c_str(), (uint)s.size());
    if (written != s.size())
    {
        UnreliableTransport::WriteError(
            TraceType,
            "TryWrite: wrote {0} bytes, expected {1}",
            s.size(),
            written);

        return ErrorCode(ErrorCodeValue::UnspecifiedError);
    }
    
    UnreliableTransport::WriteInfo(TraceType, "Updated unreliable transport config");
    return ErrorCode(ErrorCodeValue::Success);
}

void UnreliableTransportConfig::ClearNodeIdSpecifications(wstring const & nodeId)
{
    AcquireExclusiveLock lock(specsTableLock_);

    // here we just clear specifications that are of nodeId
    auto rend = remove_if(
        specifications_.begin(),
        specifications_.end(),
        [nodeId](UnreliableTransportSpecificationUPtr const & spec)
    {
        // first we need to get only the nodeId from specification (everything before first _ character)
        wstring specNodeId;
        wistringstream specName(spec->GetName());
        getline(specName, specNodeId, L'_');

        return specNodeId == nodeId;
    });
    specifications_.erase(rend, specifications_.end());
}

bool UnreliableTransportConfig::IsDisabled()
{
    return isDisabledUnreliableTransport_;
}

void UnreliableTransportConfig::LoadConfigurationFromINI(wstring const & fileName, wstring const & nodeId, bool shouldLockFile)
{
    UnreliableTransport::WriteInfo(
        TraceType, 
        "Enter UnreliableTransportConfig::LoadConfigurationFromINI, fileName='{0}', nodeId='{1}', shouldLockFile='{2}'", 
        fileName, 
        nodeId, 
        shouldLockFile);

    FileReaderLock fileLock(fileName);
        
    if (shouldLockFile)
    {               
        // acquiring lock to read file
        if (!fileLock.Acquire(TimeSpan::FromSeconds(5)).IsSuccess())
        {
            UnreliableTransport::WriteWarning(
                TraceType,
                "Failed to acquire UnreliableTransportSettings Lock file for loading. Associated file = {0}",
                fileName);
            return;
        }
    }

    // reading specifications from file
    vector<pair<wstring, wstring> > parameters;

    auto errorCode = ReadFromINI(fileName, parameters, false /* it is already locked */);
    if (!errorCode.IsSuccess())
    {
        UnreliableTransport::WriteWarning(
            TraceType,
            "ReadFromINI failed with '{0}'. Associated file = {1}",
            errorCode,
            fileName);
        return;
    }

    // removing specifications that match nodeId
    ClearNodeIdSpecifications(nodeId);
    
    // repopulating specifications with ones read from UnreliableTransportSettings.ini 
    for (auto parameter = parameters.begin();
        parameter != parameters.end();
        ++parameter)
    {
        if (!AddSpecificationNoSourceNode(nodeId, parameter->first, parameter->second))
        {
            UnreliableTransport::WriteWarning(TraceType, "Config for {0} is invalid", parameter->first);
        }
    }
    
    {
        AcquireExclusiveLock lock(specsTableLock_);
        // when INI is empty it doesn't update isUnreliableTransportDisabled_ in addSpecification
        if (parameters.size() == 0)
        {
            UpdateIsUnreliableTransportEnabled();
        }

        UnreliableTransport::WriteInfo(TraceType, "Reloaded config: {0}", *this);
    }
}

// this method should be called with lock acquired.
void UnreliableTransportConfig::UpdateIsUnreliableTransportEnabled()
{    
    if (specifications_.size() > 0)
    {
        if (isDisabledUnreliableTransport_)
        {
            UnreliableTransport::WriteInfo(TraceType, "Unreliable Transport was Enabled");
        }
            
        isDisabledUnreliableTransport_ = false;        
    }
    else
    {
        if (!isDisabledUnreliableTransport_)
        {
            UnreliableTransport::WriteInfo(TraceType, "Unreliable Transport was Disabled");
        }

        isDisabledUnreliableTransport_ = true;        
    }
}

bool UnreliableTransportConfig::AddSpecification(UnreliableTransportSpecificationUPtr && spec)
{
    RemoveSpecification(spec->GetName());

    AcquireExclusiveLock lock(specsTableLock_);
    specifications_.push_back(move(spec));

    // updating to enable unreliable transport if disabled.
    // it needs to be added here so it works with existing addbehavior in fabric client
    UpdateIsUnreliableTransportEnabled();

    return true;
}

bool UnreliableTransportConfig::AddSpecificationNoSourceNode(wstring const & nodeId, wstring const & name, wstring const & data)
{
    // creates a specification with nodeId prepended to name
    wstring nodeIdAppendedName = wformatString("{0}_{1}", nodeId, name);

    return AddSpecification(nodeIdAppendedName, data);
}

bool UnreliableTransportConfig::AddSpecification(std::wstring const & name, std::wstring const & data)
{
    // Data in ini file is stored as:
    // BehaviorName=SourceNodeId DestinationNodeID ActionName Probability Delay DelaySpan Priority ApplyCount PartitionID:IDGuid,ReplicaId:IDGuid
    StringCollection params;
    StringUtility::Split<wstring>(data, params, L" ");

    wstring source = (params.size() > 0 ? params[0] : wstring());
    wstring destination = (params.size() > 1 ? params[1] : wstring());
    wstring action = (params.size() > 2 ? params[2] : wstring());
    double probability = (params.size() > 3 ? Common::Double_Parse(params[3]) : 1.0);
    TimeSpan delay = (params.size() > 4 ? ParseTimeSpan(params[4]) : TimeSpan::MaxValue);
    TimeSpan delaySpan = (params.size() > 5 ? ParseTimeSpan(params[5]) : TimeSpan::Zero);
    int priority = (params.size() > 6 ? Common::Int32_Parse(params[6]) : 0);
    int applyCount = (params.size() > 7 ? Common::Int32_Parse(params[7]) : -1);
    wstring filterInfo = (params.size() > 8 ? params[8] : wstring());

    // construct the map for filters
    StringCollection filterList;
    StringUtility::Split<wstring>(filterInfo, filterList, L",");
    StringMap filters;

    for (auto iter = filterList.begin(); iter != filterList.end(); ++iter)
    {
        StringCollection filter;
        StringUtility::Split<wstring>(iter->data(), filter, L":");
        if (filter.size() == 2)
        {
            filters.insert(std::pair<wstring, wstring>(filter[0], filter[1]));
        }
    }

    return AddSpecification(make_unique<UnreliableTransportSpecification>(name, source, destination, action, probability, delay, delaySpan, priority, applyCount, filters));
}

bool UnreliableTransportConfig::RemoveSpecification(std::wstring const & name)
{
    AcquireExclusiveLock lock(specsTableLock_);
    auto it = remove_if(specifications_.begin(), specifications_.end(),
        [&name](UnreliableTransportSpecificationUPtr const & spec) { return spec->GetName() == name; });

    if (it == specifications_.end())
    {
        return false;
    }

    specifications_.erase(it, specifications_.end());

    // checking whether unreliable transport should be disabled.
    UpdateIsUnreliableTransportEnabled();

    return true;
}

void UnreliableTransportConfig::LoadConfiguration(Config & config)
{
    wstring section(L"UnreliableTransport");
    StringCollection specKeys;
    config.GetKeys(section, specKeys);

    for(wstring const & key : specKeys)
    {
        bool isEncrypted = false;
        wstring value = config.ReadString(section, key, isEncrypted);

        ASSERT_IF(
            isEncrypted,
            "value of SectionName={0} and KeyName={1} is encrypted. Encryption is not supported on this parameter", section, key);

        if (!AddSpecification(key, value))
        {
            UnreliableTransport::WriteWarning(TraceType, "Config for {0} is invalid", key);
        }
    }

    UnreliableTransport::WriteInfo(TraceType, "Loaded config: {0}", *this);
}

void UnreliableTransportConfig::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    for (auto it = specifications_.begin(); it != specifications_.end(); ++it)
    {
        w.Write("{0}\r\n", *it);
    }
}

DEFINE_SINGLETON_COMPONENT_CONFIG(UnreliableTransportConfig)

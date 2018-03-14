# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

"""
service fabric lldb extension

usage:
    (lldb) script import fabdbg
    (lldb) script help(fabdbg)
    (lldb) script fabdbg.function(...)
"""

import lldb
import re
import collections
import operator
import time
import multiprocessing
import sys
from joblib import Parallel, delayed


PointerByteSize = 8


def addr_to_ptr(addr) :
    # per lldb documentation, addr.load_addr should be used instead of addr.file_addr.
    # however, our exectuable is loaded twice in lldb somehow, probably a bug in lldb.
    # load_addr does not match what's stored in actuall C++ object, thus we have to 
    # use file_addr, which means it only works for types defined in executable, not
    # types defined in so files. The suggested worked around is to use target.ResolveLoadAddress(ptrValue)
    # and compare result SBAddress with vtable symbol SBAddress, but it slows things
    # down significantly, we should investigate if it is possible to speed it up
    return addr.file_addr


def vtable_addr (vtableSymbol):
    addr = addr_to_ptr(vtableSymbol.addr)
    if addr == lldb.LLDB_INVALID_ADDRESS :
        return lldb.LLDB_INVALID_ADDRESS
    return addr + 0x10 


def get_search_regions() :

    memoryRegions = lldb.process.GetMemoryRegions()
    print 'total memory regions: ', memoryRegions.GetSize()
    region = lldb.SBMemoryRegionInfo()

    searchRegions = []
    for i in range(memoryRegions.GetSize()):
        if memoryRegions.GetMemoryRegionAtIndex(i, region):
            if region.IsWritable() and region.IsReadable() and not region.IsExecutable():
                #print '[{0:x},{1:x}]'.format(region.GetRegionBase(), region.GetRegionEnd())
                searchRegions.append((region.GetRegionBase(), region.GetRegionEnd()))

    #sort regions in descending order, so that large regions get processed early 
    searchRegions.sort(key=lambda tup: tup[1] - tup[0], reverse=True)
#    for region in searchRegions :
#        print '{0:x} [{1:x}, {2:x})'.format(region[1]-region[0], region[0], region[1])

    searchRegionCount = len(searchRegions)
    print 'target memory regions: ', searchRegionCount 

    return searchRegions


def findtype_in_region(region, vtableAddr) :

    sys.stdout.write('+')
    sys.stdout.flush()

    startAddr = region[0]
    endAddr = region[1]
    #print '[{0:x},{1:x})'.format(startAddr, endAddr)

    matches = set()
    error = lldb.SBError()

    for addr in range(startAddr, endAddr, PointerByteSize):
        ptr = lldb.process.ReadPointerFromMemory(addr, error)
        if error.success and ptr == vtableAddr :
            matches.add(addr)

    sys.stdout.write('.')
    sys.stdout.flush()

    return matches


def findtype (typename):
    """
        find objects of the "virtual" type with given typename 
            for example, typename='Transport::TcpDatagramTransport'
            wll search for all TcpDatagramTransport instances
    """

    startTime = time.time()

    matchCount = 0
    vtblSymbol = 'vtable for ' + typename
    symbols = lldb.target.FindSymbols(vtblSymbol)
    if len(symbols) == 0 :
        print '%s is not a virtual type' %typename
        return

    searchRegions = get_search_regions();
    searchRegionCount = len(searchRegions)
    processorCount = multiprocessing.cpu_count()
    vtableAddr = vtable_addr(symbols[0].symbol)
    if vtableAddr == lldb.LLDB_INVALID_ADDRESS :
        print 'vtable address is LLDB_INVALID_ADDRESS'
        return

    print 'searching vtable address 0x{0:x} in target regions on {1} cores'.format(vtableAddr, processorCount)
    #print '%x' % symbols[0].symbol.addr.load_addr

    taskResults = Parallel(n_jobs=processorCount)(delayed(findtype_in_region)(searchRegions[i], vtableAddr) for i in range(searchRegionCount))

    print
    print
    print '<<<matches>>>'

    matchCount = 0
    for taskResult in taskResults :
        matchCount += len(taskResult)
        for ptr in taskResult :
            print '0x{0:x}'.format(ptr)

    print
    print 'total match found: ', matchCount
    print 'time elapsed: ', time.time() - startTime
    print


def findtypes_in_region(region, names) : 

    sys.stdout.write('+')
    sys.stdout.flush()

    startAddr = region[0]
    endAddr = region[1]
    #print '[{0:x},{1:x})'.format(startAddr, endAddr)

    matches = dict()

    error = lldb.SBError()
    for addr in range(startAddr, endAddr, PointerByteSize):
        ptr = lldb.process.ReadPointerFromMemory(addr, error)
        if error.success and ptr in names: 
            if ptr in matches :
                matches[ptr] += 1
            else :
                matches[ptr] = 1

    sys.stdout.write('.')
    sys.stdout.flush()

    return matches


def type_regex_to_vtable_regex(typename) :
    if len(typename) == 0 :
        return '^vtable for'

    if typename[0] == '^' :
        return '^vtable for ' + typename[1:]

    return '^vtable for .*' + typename


def get_all_pure_virtual_funcs() :
    result = dict()
    symbolCtxList = lldb.target.FindSymbols('__cxa_pure_virtual')
    for ctx in symbolCtxList :
        symbol = ctx.symbol
        pvfAddr = addr_to_ptr(symbol.addr)
        result[pvfAddr]=ctx.module.platform_file.basename

    """
    print 'found %d pure virtual functions:' %len(result)
    for pvf, n in result.iteritems() :
        print '%0.16x : %s' % (pvf,n)
    """

    return result


def has_pure_virtual(vtableAddr, pureVirtualFuncs) :
    error = lldb.SBError()
    vtableEndAddr = lldb.process.ReadPointerFromMemory(vtableAddr-PointerByteSize, error)
    if not error.success :
        return False 

    #print "vtable: [%0.16x, %0.16x)" % (vtableAddr, vtableEndAddr)
    for addr in range(vtableAddr, vtableEndAddr, PointerByteSize) :
        #print "read from address %.016x" % addr
        funcAddr = lldb.process.ReadPointerFromMemory(addr, error)
        if not error.success :
            continue

        if funcAddr in pureVirtualFuncs :
            return True

    return False


def findtypes (pattern, ignorePureVirtualType=True):
    """
    count objects of "virtual" types that match pattern string and rank them based on object count
        pattern: regular expression string for target types
            for example:
                pattern='' or pattern='.*' will match all virtual types
                pathern='^(?!std)' will match all non-std virtual types
                pattern='^Transport::' will match all Transport virtual types 
                pattern='Transport$' will match all virtual types ending with Transport
    """

    startTime = time.time()
    moduleCount = lldb.target.GetNumModules()
    print 'search for matching virtual types in {0} modules ...'.format(moduleCount),

    # find all virtual types first
    symbolPattern = type_regex_to_vtable_regex(pattern)
    symbolRegex = re.compile(symbolPattern) 
    names = dict()
    matches = dict()

    pureVirtualFuncs = set()
    if ignorePureVirtualType :
        pureVirtualFuncs = get_all_pure_virtual_funcs()

    for i in range(moduleCount) :
        module = lldb.target.GetModuleAtIndex(i)
        symbolCount = module.GetNumSymbols()

        for j in range(symbolCount) :
            symbol = module.GetSymbolAtIndex(j)
            symbolName = symbol.name
            if symbolName and symbolRegex.match(symbolName) :
                vtableAddr = vtable_addr(symbol)
                if vtableAddr == lldb.LLDB_INVALID_ADDRESS :
                    continue

                if ignorePureVirtualType and has_pure_virtual(vtableAddr, pureVirtualFuncs) :
                    continue

                if vtableAddr in names and not names[vtableAddr] == symbol.GetName()[11:]:
                    print 'file_addr {0:x} conflicts: {1}, {2}'.format(vtableAddr, names[vtableAddr], symbol.GetName()[11:])  
                names[vtableAddr] = symbol.GetName()[11:]
                matches[vtableAddr] = 0

    """
    for vtableAddr, symbolName in names.items() :
        print '0x{0:x} {1}'.format(vtableAddr, symbolName)
    """

    print 'found {0}'.format(len(names))
    if len(names) == 0 :
        return

    # search for matches of virtual types
    searchRegions = get_search_regions();
    searchRegionCount = len(searchRegions)
    processorCount = multiprocessing.cpu_count()
    print 'searching target regions on {0} cores'.format(processorCount)

    taskResults = Parallel(n_jobs=processorCount)(delayed(findtypes_in_region)(searchRegions[i], names) for i in range(searchRegionCount))

    for taskResult in taskResults :
        for ptr, count in taskResult.iteritems() :
            matches[ptr] += count 

    # output result
    print
    print
    print 'object count {'
    matchRanking = sorted(matches.items(), key=operator.itemgetter(1)) 
    for vtableAddr, objCount in matchRanking :
        if objCount > 0 :
            print '{0:9} {1}'.format(objCount, names[vtableAddr])

    print '} object count'
    print
    print 'time elapsed: ', time.time() - startTime
    print


def findall(pattern, ignorePureVirtualType=True):
    """
    findall is an alias of findtypes
    """
    findtypes(pattern,ignorePureVirtualType)


def findptr_in_region(region, ptrValue) :

    sys.stdout.write('+')
    sys.stdout.flush()

    startAddr = region[0]
    endAddr = region[1]
    #print '[{0:x},{1:x})'.format(startAddr, endAddr)

    matches = set()
    error = lldb.SBError()

    for addr in range(startAddr, endAddr, PointerByteSize):
        ptr = lldb.process.ReadPointerFromMemory(addr, error)
        if error.success and ptr == ptrValue:
            matches.add(addr)

    sys.stdout.write('.')
    sys.stdout.flush()

    return matches


def findptr(ptrValue) :
    """
    find pointer value or pointer size integer value
    """

    startTime = time.time()

    searchRegions = get_search_regions() 
    searchRegionCount = len(searchRegions)
    processorCount = multiprocessing.cpu_count()
    print 'searching target regions on {0} cores'.format(processorCount)

    taskResults = Parallel(n_jobs=processorCount)(delayed(findptr_in_region)(searchRegions[i], ptrValue) for i in range(searchRegionCount))

    print
    print
    print '<<<matches>>>'

    matchCount = 0
    for taskResult in taskResults :
        for match in taskResult :
            print '0x{0:x}'.format(match)
            matchCount += 1

    print
    print 'total: ', matchCount
    print 'time elapsed: ', time.time() - startTime
    print


def findsptr_in_region(region, objPtr, refCountTypeVTable) :

    sys.stdout.write('+')
    sys.stdout.flush()

    startAddr = region[0]
    endAddr = region[1]
    #print '[{0:x},{1:x})'.format(startAddr, endAddr)
    matches = set()

    error = lldb.SBError()
    for addr in range(startAddr, endAddr-PointerByteSize, PointerByteSize):
        ptr = lldb.process.ReadPointerFromMemory(addr, error)
        if error.fail :
            continue

        if ptr != objPtr :
            continue;

        ptr2 = lldb.process.ReadPointerFromMemory(addr + PointerByteSize, error)
        if error.fail :
            continue

        ptr3 = lldb.process.ReadPointerFromMemory(ptr2, error)
        if error.fail :
            continue

        if ptr3 == refCountTypeVTable :  
            matches.add(addr)
            
    sys.stdout.write('.')
    sys.stdout.flush()

    return matches


def findsptr(sptrAddr) :
    """
        find shared_ptr or weak_ptr instances of a given object by matching
        both pointer of shared object and vtable of shared_ptr/weak_ptr ref count type
            strAddr: address of a shared_ptr/weak_ptr instance of the given object
    """

    startTime = time.time()

    error = lldb.SBError()
    objPtr = lldb.process.ReadPointerFromMemory(sptrAddr, error)
    if error.fail :
        print 'failed to read from ', sptrAddr, ' : ', error
        return

    print 'address of shared object: 0x{0:x}'.format(objPtr)

    refCountObjPtr = lldb.process.ReadPointerFromMemory(sptrAddr + PointerByteSize, error)
    if error.fail :
        print 'failed to read from {0}: {1}'.format(sptrAddr+PointerByteSize, error)
        return

    print 'address of shared_ptr ref count object: 0x{0:x}'.format(refCountObjPtr)

    refCountTypeVTable = lldb.process.ReadPointerFromMemory(refCountObjPtr, error)
    if error.fail :
        print 'failed to read vtable address of shared_ptr ref count type : ', error
        return

    print 'vtable address of shared_ptr ref count type: 0x{0:x}'.format(refCountTypeVTable)

    searchRegions = get_search_regions() 
    searchRegionCount = len(searchRegions)
    processorCount = multiprocessing.cpu_count()
    print 'searching target regions on {0} cores'.format(processorCount)

    taskResults = Parallel(n_jobs=processorCount)(delayed(findsptr_in_region)(searchRegions[i], objPtr, refCountTypeVTable) for i in range(searchRegionCount))

    print
    print
    print '<<<matches>>>'
    matchCount = 0
    for taskResult in taskResults :
        for match in taskResult :
            print '0x{0:x}'.format(match)
            matchCount += 1

    print
    print 'total: ', matchCount
    print 'time elapsed: ', time.time() - startTime
    print


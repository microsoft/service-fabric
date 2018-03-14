# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------

import lldb

def wstring_SummaryProvider(obj, dict):
    """
    dump wstring with 16bit characters
    (lldb) script import sftypes.py 
    (lldb) type summary add -F sftypes.wstring_SummaryProvider 'std::__1::wstring'
    (lldb) print some_wstring_variable
    """
    result='"'
    e = lldb.SBError()

    first = obj.GetChildMemberWithName('__r_').GetChildMemberWithName('__first_') 
    l = first.GetChildMemberWithName('__l')
    bufferAddr = l.GetChildMemberWithName('__data_').GetValueAsUnsigned()
    wcharCount = l.GetChildMemberWithName('__size_').GetValueAsUnsigned()

    if bufferAddr > 8 and wcharCount > 0 :
        byteCount = wcharCount * 2 
        content = obj.process.ReadMemory(bufferAddr, byteCount, e)
        if not e.success :
            result = e.description
            return result 
        
        for i in range(wcharCount):
            byteIdx = i * 2
            result += content[byteIdx]
    else :
        s = first.GetChildMemberWithName('__s')
        size = s.GetChildMemberWithName('__size_').GetData().GetUnsignedInt8(e, 0)

        if e.success :
            if size > 0 :
                data = s.GetChildMemberWithName('__data_').GetData()
                for i in range(0, size, 2) :
                    result += chr(data.uint8[i])
        else :
            result = e.description
            return result 

    result+='"'
    return result


"""
def doLoad(debugger, dictionary) :
    debugger.HandleCommand('type summary add -F sftypes.wstring_SummaryProvider "std::__1::wstring"')


def __lldb_init_module(debugger, dictionary):
    doLoad(debugger, dictionary)
"""

/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kdomutil.cpp

    Description:
      Kernel Tempate Library (KTL): KDomPath, KDomDoc

      Wrappers and helpers for KDom objects.

    History:
      raymcc          19-Jul-2012

--*/


BOOLEAN
KDomDoc::Set(
    __in KDomPath& Path,
    __in KVariant& Val
    )
{
    KIMutableDomNode::SPtr Node = GetNode(Path);
    return TRUE;
}





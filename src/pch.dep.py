#!/usr/bin/python

# ------------------------------------------------------------
# Copyright (c) Microsoft Corporation.  All rights reserved.
# Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
# ------------------------------------------------------------


import commands 
import os 

scriptDir = os.path.dirname(os.path.realpath(__file__))
os.chdir(scriptDir)

def GetOutputLines(cmd):
    output = commands.getstatusoutput(cmd)
    if output[0] != 0: return [] 
    return output[1].splitlines()

gsResult = GetOutputLines("git status -s *.h")

updatedHeaderFiles = set() 
for line in gsResult[:]:
    split = line.split()
    for entry in split[:]:
        if entry.endswith(".h"):
            updatedHeaderFiles.add(entry)

print os.getcwd(), 'has the following updated header files:'
for path in updatedHeaderFiles:
    print "  ", path
print

def GetGrepTarget(path):
    grepTarget = os.path.basename(path)
    lowercase = path.lower()
    if lowercase.endswith("stdafx.h"):
        parent = os.path.dirname(path)
#        print 'parent: ',parent
        pathSegments = parent.split('/')
#        print 'pathSegments: ',pathSegments
        if len(pathSegments) > 0:
            grepTarget = os.path.join(pathSegments[-1], grepTarget)
            print "joined grep target: ",grepTarget

    return grepTarget
 
grepTargets = set() 
for path in updatedHeaderFiles:
    grepTargets.add(GetGrepTarget(path))

print "initial grep targets:"
for grepTarget in grepTargets:
    print "  ", grepTarget
print

touchTargetCount = 0
touchTargets = set() 

while True:
    newTouchTargets = set()
    for grepTarget in grepTargets:
        cmd = "grep -l -s " + grepTarget + " -r . --include=*.h"
        print cmd
        output = GetOutputLines(cmd)
        print output
        if len(output) == 0 : continue

        for line in output[:]: 
           touchTargets.add(line)
           newTouchTargets.add(line)

    if len(touchTargets) == touchTargetCount : break
    touchTargetCount = len(touchTargets)

    grepTargets = set()
    for newTouchTarget in newTouchTargets:
       grepTargets.add(GetGrepTarget(newTouchTarget))

print
print "touch affected precompiled headers:"
for touchTarget in touchTargets:
   cmd = 'touch ' + touchTarget
   print '  ', cmd, commands.getstatusoutput(cmd)
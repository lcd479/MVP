#!/usr/bin/python2.7

#/****************************************************************************
#*                                                                           *
#*  OpenNI 2.x Alpha                                                         *
#*  Copyright (C) 2012 PrimeSense Ltd.                                       *
#*                                                                           *
#*  This file is part of OpenNI.                                             *
#*                                                                           *
#*  Licensed under the Apache License, Version 2.0 (the "License");          *
#*  you may not use this file except in compliance with the License.         *
#*  You may obtain a copy of the License at                                  *
#*                                                                           *
#*      http://www.apache.org/licenses/LICENSE-2.0                           *
#*                                                                           *
#*  Unless required by applicable law or agreed to in writing, software      *
#*  distributed under the License is distributed on an "AS IS" BASIS,        *
#*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
#*  See the License for the specific language governing permissions and      *
#*  limitations under the License.                                           *
#*                                                                           *
#****************************************************************************/
from __future__ import print_function

import os
import re
import sys
import shutil
import subprocess
import platform
import argparse
import stat

import UpdateVersion
from Harvest import Harvest

if len(sys.argv) < 2 or sys.argv[1] in ('-h','--help'):
    print("usage: " + sys.argv[0] + " <x86|x64|Arm|Android> [UpdateVersion]")
    sys.exit(1)

plat = sys.argv[1]
origDir = os.getcwd()

shouldUpdate = 0
if len(sys.argv) >= 3 and sys.argv[2] == 'UpdateVersion':
    shouldUpdate = 1

if shouldUpdate == 1:
    # Increase Build
    UpdateVersion.VERSION_BUILD += 1
    UpdateVersion.update()

def check_call(cmd, outputFile = None):
    if platform.system() == 'Windows':
        useShell=True
    else:
        useShell=False

    rc = subprocess.call(cmd, shell=useShell, stdout=outputFile, stderr=outputFile)
    if rc != 0:
        print('Failed to execute command: ' + str(cmd))
        sys.exit(9)

def get_reg_values(reg_key, value_list):
    # open the reg key
    try:
        reg_key = win32api.RegOpenKeyEx(*reg_key)
    except pywintypes.error as e:
        raise Exception("Failed to open registry key!")
    # Get the values
    try:
        values = [(win32api.RegQueryValueEx(reg_key, name), data_type) for name, data_type in value_list]
        # values list of ((value, type), expected_type)
        for (value, data_type), expected in values:
            if data_type != expected:
                raise Exception("Bad registry value type! Expected %d, got %d instead." % (expected, data_type))
        # values okay, leave only values
        values = [value for ((value, data_type), expected) in values]
    except pywintypes.error as e:
        raise Exception("Failed to get registry value!")
    finally:
        try:
            win32api.RegCloseKey(reg_key)
        except pywintypes.error as e:
            # We don't care if reg key close failed...
            pass
    return tuple(values)

def calc_jobs_number():
    cores = 1

    try:
        if isinstance(self, OSMac):
            txt = gop('sysctl -n hw.physicalcpu')
        else:
            txt = gop('grep "processor\W:" /proc/cpuinfo | wc -l')

        cores = int(txt)
    except:
        pass

    return str(cores * 2)

# Create installer
strVersion = UpdateVersion.getVersionName()
print("Creating installer for OpenNI " + strVersion + " " + plat)
finalDir = "Final"
if not os.path.isdir(finalDir):
    os.mkdir(finalDir)

if platform.system() == 'Linux' or platform.system() == 'Darwin':

    devNull = open('/dev/null', 'w')
    subprocess.check_call(['sudo', 'make', '-C', '../', '-j' + calc_jobs_number(), 'PLATFORM=' + plat, 'HAS_JAVA=1', 'clean'], stdout=devNull, stderr=devNull)
    devNull.close()

    buildLog = open(origDir + '/build.release.' + plat + '.log', 'w')
    subprocess.check_call(['sudo', 'make', '-C', '../', '-j' + calc_jobs_number(), 'PLATFORM=' + plat, 'HAS_JAVA=1', 'release'], stdout=buildLog, stderr=buildLog)
    buildLog.close()

    # everything OK, can remove build log
    os.remove(origDir + '/build.release.' + plat + '.log')

else:
    print("Unknown OS")
    sys.exit(2)

# also copy Release Notes and CHANGES documents
shutil.copy('../ReleaseNotes.txt', finalDir)
shutil.copy('../CHANGES.txt', finalDir)

print("Installer can be found under: " + finalDir)
print("Done")

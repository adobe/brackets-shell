#!/usr/bin/env python

# Original file: http://code.google.com/searchframe#OAMlx_jo-ck/src/tools/grit/grit/format/data_pack.py
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
# (http://code.google.com/searchframe#OAMlx_jo-ck/src/LICENSE)

# This file:
#
# Copyright (c) 2012 Adobe Systems Incorporated. All rights reserved.
#  
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"), 
# to deal in the Software without restriction, including without limitation 
# the rights to use, copy, modify, merge, publish, distribute, sublicense, 
# and/or sell copies of the Software, and to permit persons to whom the 
# Software is furnished to do so, subject to the following conditions:
#  
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#  
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
# DEALINGS IN THE SOFTWARE.


'''Support for formatting a data pack file used for platform agnostic resource
files.
'''

import collections
import exceptions
import os
import struct
import sys
import re
import shutil

PACK_FILE_VERSION = 4
HEADER_LENGTH = 2 * 4 + 1  # Two uint32s. (file version, number of entries) and
                           # one uint8 (encoding of text resources)
BINARY, UTF8, UTF16 = range(3)


class WrongFileVersion(Exception):
  pass


DataPackContents = collections.namedtuple('DataPackContents', 'resources encoding')


def ReadDataPack(input_file):
  """Reads a data pack file and returns a dictionary."""
  with open(input_file, "rb") as file:
    data = file.read()
  original_data = data

  # Read the header.
  version, num_entries, encoding = struct.unpack("<IIB", data[:HEADER_LENGTH])
  if version != PACK_FILE_VERSION:
    print "Wrong file version in ", input_file
    raise WrongFileVersion

  resources = {}
  if num_entries == 0:
    return DataPackContents(resources, encoding)

  # Read the index and data.
  data = data[HEADER_LENGTH:]
  kIndexEntrySize = 2 + 4  # Each entry is a uint16 and a uint32.
  for _ in range(num_entries):
    id, offset = struct.unpack("<HI", data[:kIndexEntrySize])
    data = data[kIndexEntrySize:]
    next_id, next_offset = struct.unpack("<HI", data[:kIndexEntrySize])
    resources[id] = original_data[offset:next_offset]

  return DataPackContents(resources, encoding)


def WriteDataPackToString(resources, encoding):
  """Write a map of id=>data into a string in the data pack format and return
  it."""
  ids = sorted(resources.keys())
  ret = []

  # Write file header.
  ret.append(struct.pack("<IIB", PACK_FILE_VERSION, len(ids), encoding))
  HEADER_LENGTH = 2 * 4 + 1            # Two uint32s and one uint8.

  # Each entry is a uint16 + a uint32s. We have one extra entry for the last
  # item.
  index_length = (len(ids) + 1) * (2 + 4)

  # Write index.
  data_offset = HEADER_LENGTH + index_length
  for id in ids:
    ret.append(struct.pack("<HI", id, data_offset))
    data_offset += len(resources[id])

  ret.append(struct.pack("<HI", 0, data_offset))

  # Write data.
  for id in ids:
    ret.append(resources[id])
  return ''.join(ret)


def WriteDataPack(resources, output_file, encoding):
  """Write a map of id=>data into output_file as a data pack."""
  content = WriteDataPackToString(resources, encoding)
  with open(output_file, "wb") as file:
    file.write(content)

def pack(directory, pakFile):
  print "Packing %s as %s" % (directory, pakFile)
  
  files = os.listdir(directory)
  files.sort()
  
  numeric = re.compile("^\d+$")
  
  data = {}
  for (id) in files:
    if not numeric.match(id):
      continue
    input_file = "%s/%s" % (directory, id)
    with open(input_file, "rb") as file:
      data[int(id)] = file.read()

  #~ data = { 1: "", 4: "this is id 4", 6: "this is id 6", 10: "" }
  WriteDataPack(data, pakFile, UTF8)

def unpack(pakFile, directory):
  print "Unpacking %s to %s" % (pakFile, directory)

  if os.path.exists(directory):
    shutil.rmtree(directory)
  os.makedirs(directory)

  data = ReadDataPack(pakFile)
  #print data.encoding
  for (resource_id, contents) in data.resources.iteritems():
    output_file = "%s/%s" % (directory, resource_id)
    with open(output_file, "wb") as file:
      file.write(contents)

def findIdForName(headerFile, name):
  print "Extracting ID for %s from header file %s" % (name, headerFile)
  with open(headerFile, "rb") as file:
    match = re.search("#define %s (\d+)" % (name), file.read())
    return int(match.group(1)) if match else None 

def patch(devtools_js):
  print "Patching %s" % devtools_js
  with open(devtools_js, "rb") as file:
    contents = file.read()
    
    # in:  this.cacheDisabled = this.createSetting("cacheDisabled", false);
    # out: this.cacheDisabled = this.createSetting("cacheDisabled", true);
    contents = re.sub("(this\.createSetting\s*\((['\"])cacheDisabled\\2,\s*)false(\s*\))", "\\1true\\3", contents)
    
    # delete:  if (WebInspector.settings.cacheDisabled.get())
    #          NetworkAgent.setCacheDisabled(true);
    contents = re.sub("if\s*\(WebInspector\.settings\.cacheDisabled\.get\s*\(\)\s*\)\s*[\s\r\n]*NetworkAgent\.setCacheDisabled\s*\(true\)\s*;?[\s\r\n]*", "", contents)
    
    # find: _reset: function(preserveItems)
    #       {
    # add:  if (WebInspector.settings.cacheDisabled.get())
    #         NetworkAgent.setCacheDisabled(true);
    contents = re.sub("(_reset:\s*function\s*\(\s*preserveItems\s*\)[\s\r\n]*\{[\s\r\n]*)", "\\1if (WebInspector.settings.cacheDisabled.get())\n{\nwindow.setTimeout(function() { NetworkAgent.setCacheDisabled(true); }, 1000);\n}", contents)
  
  with open(devtools_js, "wb") as file:
    file.write(contents)

def patchDevTools():
  root = os.path.normpath(os.path.join(os.path.dirname(__file__), '..'))
  directories = [os.path.join(root, "Release"), os.path.join(root, "Debug")]

  headerFile = os.path.join(root, "include", "cef_pack_resources.h")
  id = findIdForName(headerFile, "DEVTOOLS_JS")
  
  for (buildType) in ["Release", "Debug"]:
    directory  = os.path.join(root, buildType)
    original   = os.path.join(directory, "devtools_resources.pak.bak")
    target     = os.path.join(directory, "devtools_resources.pak")
    tempDir    = os.path.join(directory, "devtools_resources")
    
    if not os.path.exists(original):
      shutil.copy2(target, original)
    
    unpack(original, tempDir)
    patch(os.path.join(tempDir, str(id)))
    pack(tempDir, target)
    
    shutil.rmtree(tempDir)

def main():
  if len(sys.argv) <= 1:
    patchDevTools()
  else:
    path = sys.argv[1]
    if os.path.isdir(path):
      pack(path, path + ".pak")
    else:
      unpack(path, re.sub('\.pak$', '', path))

if __name__ == '__main__':
  main()
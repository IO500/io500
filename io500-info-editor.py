#!/usr/bin/env python
import sys
import json

# This tool allows to view and edit the options in an existing schema file.
# In order to get started download the two files:
# wget https://www.vi4io.org/lib/plugins/newcdcl/scripts/schema-io500.json
# wget https://www.vi4io.org/lib/plugins/newcdcl/scripts/site-io500.json
#
# You can then copy the site-io500.json template file and modify it using this tool

if len(sys.argv) < 3:
  print("synopsis: %s <FILE> {<TOKEN[=VALUE]>}" % sys.argv[0])
  print("examples:")
  print("printing current: %s site.json Site.institution" % sys.argv[0])
  print("Token examples:")
  print("changing: Site.StorageSystem.Lustre.OSS.count=5")
  print("changing options: Site.StorageSystem.Lustre.features=DNE1;DNE2")
  print("changing a numeric value \"Site.IO500.IOR.easy write\" = [351.2,GiB/s]")
  sys.exit(1)

file = sys.argv[1]

def processDict(data, token, path, val):
  cur = token[0].strip()
  if len(token) == 1:
    if val != None:
      if val[0] == '[':
        tmp = val.split(",")
        val = [ float(tmp[0][1:]), tmp[1][0:-1] ]
      data["att"][cur] = val
    if cur in data["att"]:
      print("%s = %s" % (path, data["att"][cur]))
    else:
      print("%s = undefined" % path)
  else:
    processSingle(data["childs"], token, path, val)

def processSingle(data, token, path, val):
  cur = token.pop(0)
  if isinstance(data, list):
    # need to find token
    for k in range(0, len(data)):
      if data[k]["type"] == cur:
        processDict(data[k], token, path, val)
        return
    data.append({"type" : cur, "att" : {}, "childs" : [] })
    processDict(data[len(data)-1], token, path, val)
    return
  if data["type"] == cur:
    processDict(data, token, path, val)

def process(data, tokens):
  for t in tokens:
    kv = t.split("=")
    token = kv[0].split(".")
    processSingle(data, token, kv[0].strip(), kv[1].strip() if len(kv) == 2 else None)

with open(file, 'r+') as f:
    data = json.load(f)
    process(data["DATA"], sys.argv[2:])

    f.seek(0)
    json.dump(data, f, indent=2)
    f.truncate()


print("OK")

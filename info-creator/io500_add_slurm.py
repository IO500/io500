#!/usr/bin/env python3

# Identify information from SLURM and add them into the result JSON file

import sys
import platform
import os
import re
import traceback
import subprocess
from io500_info_editor import edit_infos, execute_download

if len(sys.argv) < 2:
  print("Synopsis: %s <SYSTEM-JSON> [<SupercomputerTypeNumber>]" % sys.argv[0])
  sys.exit(1)

json = sys.argv[1]
supercomputerNumber = sys.argv[2] if len(sys.argv) > 2 else "0"

cmd = []

def infoS(key, val, unit = ""):
  global cmd, supercomputerNumber
  val = str(val).strip()
  unit = unit.strip()
  if val == None or val == "":
    return
  cmd.append(("Site.Supercomputer[%s]." % (supercomputerNumber) + key + "=" + val + " " + unit).strip())

def info(nodeNumber, key, val, unit = ""):
  infoS("Nodes[%s]." % (nodeNumber) + key, val)

try:
  #data = subprocess.check_output("LANG=C sinfo -o \"%P %.6D %f %z\"", shell=True, universal_newlines=True).strip()
  data = open("sinfo.txt").read()
  nodeNumber = 0
  totalNodes = 0
  for line in data.split("\n"):
    m = re.match("([^ ]+)[ ]+([^ ]+)[ ]+([^ ]+)[ ]+([0-9]+):([0-9]+):([0-9]+)", line)
    if m:
      name = m.group(1)
      count = m.group(2)
      features = m.group(3)
      try:
        count = int(count)
        info(nodeNumber, "name", name + "_" + features)
        info(nodeNumber, "count", count)
        info(nodeNumber, "Processor.sockets", m.group(4))
        info(nodeNumber, "Processor.cores per socket", m.group(5))
        info(nodeNumber, "Processor.threads per core", m.group(6))

        nodeNumber = nodeNumber + 1
        totalNodes = totalNodes + count
      except:
        pass
  infoS("total nodes", totalNodes)
except:
  traceback.print_exc()
  print("Cannot execute sinfo")


#print(cmd)
edit_infos(json, cmd)

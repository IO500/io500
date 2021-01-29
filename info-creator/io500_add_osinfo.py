#!/usr/bin/env python3

# Identify OS-specific information and add them into the result JSON file

import sys
import platform
import subprocess
import os
import re
from io500_info_editor import edit_infos

if len(sys.argv) < 2:
  print("Synopsis: %s <SYSTEM-JSON>" % sys.argv[0])
  sys.exit(1)

json = sys.argv[1]
cmd = []

def info(key, val, unit = ""):
  global cmd
  val = str(val).strip()
  unit = unit.strip()
  if val == None or val == "":
    return
  cmd.append(("Site." + key + "=" + val + " " + unit).strip())

info("Supercomputer.Nodes.name", platform.node())
info("Supercomputer.Nodes.kernel version", platform.release())
info("Supercomputer.Nodes.Processor.architecture", platform.processor())

# CPU Information
data = open("/proc/cpuinfo", "r").read()
model_set = False
cores = 0
for line in data.split("\n"):
  if "model name" in line and not model_set:
    m = re.match("model name.*: ([^@]*)(@ ([0-9.]*)GHz)?", line)
    if m:
      cpu = m.group(1).strip()
      if cpu.find("CPU") > -1:
        cpu = cpu.replace("CPU", "")
      cpu = re.sub("\([^)]*\)", "", cpu)
      info("Supercomputer.Nodes.Processor.model", cpu)
      info("Supercomputer.Nodes.Processor.frequency", m.group(3), "GHz")
      model_set = True
  if line.startswith("processor"):
    cores = cores + 1

def re_add(regex, key, data, group = 1):
  m = re.search(regex, data)
  if m:
    info(key, m.group(group))

try:
  data = subprocess.check_output("LANG=C lscpu", shell=True, universal_newlines=True).strip()
  m = re.search("Core\(s\) per socket: *([0-9]+)", data)
  if m:
    cores = m.group(1)
  re_add("Thread\(s\) per core: *([0-9]+)", "Supercomputer.Nodes.Processor.threads per core", data)
  
  re_add("Vendor ID: *(.*)", "Supercomputer.Nodes.Processor.vendor", data)
  re_add("L2 cache: *([0-9]+.*)", "Supercomputer.Nodes.Processor.L2 cache size", data)
  re_add("L3 cache: *([0-9]+.*)", "Supercomputer.Nodes.Processor.L3 cache size", data)

except:
  print("Cannot execute lscpu, will continue")

info("Supercomputer.Nodes.Processor.cores per socket", cores)

# OS Information
kv = {}
with open("/etc/os-release") as f:
  for line in f:
    arr = line.split("=")
    kv[arr[0].strip()] = arr[1].strip(" \n\t\"")

if "ID" in kv:
  val = kv["ID"]
  if val == "ubuntu":
    val = "Ubuntu"
  info("Supercomputer.Nodes.distribution", val)
if "VERSION_ID" in kv:
  info("Supercomputer.Nodes.distribution version", kv["VERSION_ID"])

# Try to find country code using a reverse address

if not os.path.exists("ip.html"):
  try:
    res = subprocess.check_output("wget https://pbxbook.com/other/where_ip.html -O ip.html", shell=True, universal_newlines=True).strip()
    with open("ip.html") as f:
      m = re.search("public IP:.*Country:</b> .* / (.*) /", f.read())
      if m:
        info("nationality", m.group(1))
  except:
    print("Cannot execute wget, will continue")

edit_infos(json, cmd)

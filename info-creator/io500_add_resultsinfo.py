#!/usr/bin/env python3

# Parse the IO500 results and load them into the result JSON file

import sys
import configparser
from io500_info_editor import edit_infos

if len(sys.argv) < 3:
  print("Synopsis: %s <SYSTEM-JSON> <RESULT-DIR>" % sys.argv[0])
  sys.exit(1)

# results directory
json = sys.argv[1]
result = sys.argv[2]

config = configparser.ConfigParser()
config.read(result + '/result.txt')

def add_res(key, ini_section, unit, ini_key = "score"):
  global config, cmd

  cmd.append(key + "=" + config[ini_section][ini_key] + " " + unit)

cmd = []
for op in ["easy write", "easy read", "hard write", "hard read"] :
  t = op.replace(" ", "-")
  add_res("Site.IO500.IOR." + op, "ior-" + t, "GiB/s")

for op in ["easy write", "easy stat", "easy delete", "hard write", "hard stat", "hard delete", "hard read" ] :
  t = op.replace(" ", "-")
  add_res("Site.IO500.MDTest." + op, "mdtest-" + t, "kOP/s")

add_res("Site.IO500.find.mixed", "find", "kOP/s")

add_res("Site.IO500.score", "SCORE", "", "SCORE")
add_res("Site.IO500.scoreMD", "SCORE", "kOP/s", "MD")
add_res("Site.IO500.scoreBW", "SCORE", "GiB/s", "BW")

edit_infos(json, cmd)

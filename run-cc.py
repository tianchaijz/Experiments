#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import subprocess
import os
import sys

LUA_PATH = os.path.expandvars("$HOME/openresty/lua/src")
CC_ARGUMENTS = [
    'cc',
    '-g',
    '-O0',
    '-Wall',
    '-Werror',
    '-Wno-unused',
    '-Wpointer-arith',
]

program = sys.argv[1]
assert program, "program required"

parts = program.split('.')
suffix = parts[-1].lower()
assert suffix in ["c", "cc", "cpp"], "C/C++ program required"

name = parts[0]
if suffix == "cc" or suffix == "cpp":
    CC_ARGUMENTS[0] = "g++"
    CC_ARGUMENTS.append("--std=c++11")

if "lua_State" in open(program, 'r').read():
    CC_ARGUMENTS.append("-I%s" % LUA_PATH)
    CC_ARGUMENTS.append("-L%s" % LUA_PATH)
    CC_ARGUMENTS.append("-llua5.1")
    CC_ARGUMENTS.append("-Wl,-rpath,%s" % LUA_PATH)

CC_ARGUMENTS.append("-o")
CC_ARGUMENTS.append(name + ".out")
CC_ARGUMENTS.append(program)

command = CC_ARGUMENTS
print(' '.join(command))

process = subprocess.Popen(command,
                           stderr=subprocess.PIPE,
                           stdout=subprocess.PIPE)
code = process.wait()
stderr = process.stderr.readlines()
stdout = process.stdout.readlines()

sys.stdout.writelines(stdout)

process.stdout.close()
process.stderr.close()

if code != 0:
    sys.stderr.writelines(stderr)
    sys.exit(code)

sys.exit(0)

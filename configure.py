API_VERSION = '2.1'

import sys
try:
  from ambuild2 import run
  if not run.HasAPI(API_VERSION):
    raise Exception()
except:
  sys.stderr.write('AMBuild {0} must be installed to build this project.\n'.format(API_VERSION))
  sys.stderr.write('http://www.alliedmods.net/ambuild\n')
  sys.exit(1)

builder = run.BuildParser(sourcePath = sys.path[0], api=API_VERSION)

# Inspired by SourceMod
builder.options.add_option('--hl2sdk-path', type=str, dest='hl2sdk_path', default=None, help='Path to CSGO HL2SDK')
builder.options.add_option('--mms-path', type=str, dest='mms_path', default=None, help='Path to Metamod:Source')
builder.options.add_option('--sm-path', type=str, dest='sm_path', default=None, help='Path to SourceMod')

builder.Configure()

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regression: full image reads from regular files and fragmented stdin pipes."""
import json
import os
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]
HELPER=Path(os.environ.get('WETYPE_NATIVE_CLIPBOARD_HELPER', str(ROOT/'build/fcitx5-wetypex-clipboard-set')))
env={**os.environ,'QT_QPA_PLATFORM':'offscreen','QT_QPA_PLATFORMTHEME':'',
     'QT_STYLE_OVERRIDE':'Fusion','QT_IM_MODULE':'none'}
command=[str(HELPER),'--normalize-image']
jpeg=(ROOT/'fixtures/formats/pipe-large.jpg').read_bytes()
assert len(jpeg)>262144
with (ROOT/'fixtures/formats/pipe-large.jpg').open('rb') as f:
    expected=subprocess.run(command,stdin=f,capture_output=True,env=env,timeout=15)
assert expected.returncode==0
checks={}
for label,data,reference in [('jpeg',jpeg,expected.stdout),('png',expected.stdout,expected.stdout)]:
    normal=subprocess.run(command,input=data,capture_output=True,env=env,timeout=15)
    checks[label+'_pipe_matches_file']=normal.returncode==0 and normal.stdout==reference
    process=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.DEVNULL,env=env)
    try:
        for offset in range(0,len(data),4096):
            process.stdin.write(data[offset:offset+4096]);process.stdin.flush();time.sleep(.001)
        process.stdin.close();process.stdin=None
        output,_=process.communicate(timeout=15)
        checks[label+'_fragmented_pipe_matches_file']=process.returncode==0 and output==reference
    finally:
        if process.poll() is None:process.kill();process.wait()
oversize=subprocess.run(command,input=b'x'*(8388608+1),capture_output=True,env=env,timeout=15)
checks['oversize_rejected']=oversize.returncode==2 and not oversize.stdout
empty=subprocess.run(command,input=b'',capture_output=True,env=env,timeout=15)
checks['empty_rejected']=empty.returncode==2 and not empty.stdout
report={'synthetic_jpeg_bytes':len(jpeg),'checks':checks,'passed':all(checks.values())}
print(json.dumps(report))
assert report['passed']

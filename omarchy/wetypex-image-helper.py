#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 wetypex-omarchy contributors
"""WeTypeX 的图片适配；保留原 Qt 剪贴板接口，补 HEIF/AVIF 静态预览。"""
import os
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
HELPER=Path(os.environ.get('WETYPE_NATIVE_CLIPBOARD_HELPER', str(ROOT/'bin/fcitx5-wetypex-clipboard-set')))
if sys.argv[1:]!=['--normalize-image']:
    os.execv(str(HELPER),[str(HELPER),*sys.argv[1:]])
data=sys.stdin.buffer.read(8388609)
if not data or len(data)>8388608:raise SystemExit(2)
COMPAT=ROOT/'share/fcitx5-wetypex/compat/cloud'
if not COMPAT.is_dir(): COMPAT=ROOT/'compat/cloud'
sys.path.insert(0,str(COMPAT))
from 图片格式 import sniff_image,convert_png,ImageConversionError
mime=sniff_image(data)
if mime in ('image/heic','image/avif'):
    try:data=convert_png(data,mime)
    except ImageConversionError:raise SystemExit(2)
    if len(data)>8388608:raise SystemExit(2)
    sys.stdout.buffer.write(data)
else:
    # 纯解码不需要连接桌面；避免 Qt 的 GTK 主题初始化间接访问显示服务。
    env={**os.environ,'QT_QPA_PLATFORM':'offscreen','QT_QPA_PLATFORMTHEME':'','QT_STYLE_OVERRIDE':'Fusion','QT_IM_MODULE':'none'}
    try:
        result=subprocess.run([str(HELPER),'--normalize-image'],input=data,capture_output=True,env=env,timeout=12)
    except (OSError,subprocess.TimeoutExpired):raise SystemExit(2)
    if result.returncode or len(result.stdout)>8388608 or sniff_image(result.stdout)!='image/png':raise SystemExit(2)
    sys.stdout.buffer.write(result.stdout)

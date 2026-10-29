# Source: zhl-HFUT/vtype-linux; see ../NOTICE and ../LICENSE.
# Omarchy source export with local compatibility changes: 2026-10-29.
"""常见位图的剪贴板兼容转换；文件内容识别，不依据扩展名。

JPEG/PNG 原字节保留。其他位图以 PNG 静态预览互通；转换在限时子进程内完成。
"""
import os
import struct
import subprocess
import sys

MAX_INPUT = 16 * 1024 * 1024
MAX_OUTPUT = 10 * 1024 * 1024
MAX_SIDE = 2560
ALIASES = {'image/jpg': 'image/jpeg', 'image/pjpeg': 'image/jpeg',
           'image/x-png': 'image/png', 'image/heif': 'image/heic',
           'image/tif': 'image/tiff', 'image/x-tiff': 'image/tiff',
           'image/x-bmp': 'image/bmp', 'image/x-ms-bmp': 'image/bmp'}
LOADERS = {'image/heic': 'heic', 'image/avif': 'avif', 'image/webp': 'webp',
           'image/gif': 'gif', 'image/tiff': 'tiff', 'image/bmp': 'bmp'}
IMAGE_MIMES = ('image/png', 'image/jpeg', 'image/jpg', 'image/pjpeg', 'image/x-png',
               'image/heic', 'image/heif', 'image/avif', 'image/webp',
               'image/gif', 'image/tiff', 'image/tif', 'image/x-tiff',
               'image/bmp', 'image/x-bmp', 'image/x-MS-bmp')


class ImageConversionError(RuntimeError):
    """图像损坏、缺少解码器、超限或转换超时；异常不含正文。"""


def canonical_mime(mime):
    mime = mime.split(';', 1)[0].strip().lower()
    return ALIASES.get(mime, mime)


def sniff_image(data):
    if data.startswith(b'\x89PNG\r\n\x1a\n') and data.endswith(b'IEND\xaeB`\x82'):
        return 'image/png'
    if data.startswith(b'\xff\xd8\xff') and data.rstrip(b'\0').endswith(b'\xff\xd9'):
        return 'image/jpeg'
    if data[:6] in (b'GIF87a', b'GIF89a') and len(data) > 13 and data.endswith(b';'):
        return 'image/gif'
    if data[:4] == b'RIFF' and data[8:12] == b'WEBP' and len(data) >= 20:
        if struct.unpack_from('<I', data, 4)[0] + 8 == len(data):
            return 'image/webp'
    if data[:4] in (b'II*\0', b'MM\0*', b'II+\0', b'MM\0+') and len(data) >= 16:
        return 'image/tiff'
    if data[:2] == b'BM' and len(data) >= 26 and struct.unpack_from('<I', data, 2)[0] == len(data):
        return 'image/bmp'
    # ISO BMFF 的 ftyp：只接受完整盒子中的已知静态图片品牌，不把 MP4 当 HEIF。
    if len(data) >= 24 and data[4:8] == b'ftyp':
        size = struct.unpack_from('>I', data)[0]
        if 16 <= size <= min(len(data), 4096) and size % 4 == 0:
            brands = {data[8:12], *(data[i:i+4] for i in range(16, size, 4))}
            if brands & {b'avif', b'avis'}:
                return 'image/avif'
            if brands & {b'heic', b'heix', b'hevc', b'hevx', b'mif1', b'msf1'}:
                return 'image/heic'
    return None


def convert_png(data, mime):
    mime = canonical_mime(mime)
    if mime not in LOADERS or not data or len(data) > MAX_INPUT or sniff_image(data) != mime:
        raise ImageConversionError('图片格式或大小不符合转换条件')
    try:
        result = subprocess.run([sys.executable, os.path.abspath(__file__), mime], input=data,
                                stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=10)
    except (OSError, subprocess.TimeoutExpired):
        raise ImageConversionError('图片转换不可用或超时') from None
    if result.returncode or len(result.stdout) > MAX_OUTPUT or sniff_image(result.stdout) != 'image/png':
        raise ImageConversionError('图片解码失败、缺少解码器或转换后超过10MiB')
    return result.stdout


def compatible_image(data, declared=None):
    """返回可互通的 (字节, MIME)；不支持的签名或声明不符返回 None。"""
    mime = sniff_image(data)
    if not mime or declared and canonical_mime(declared) != mime:
        return None
    if mime in ('image/png', 'image/jpeg'):
        return data, mime
    return convert_png(data, mime), 'image/png'


def worker(data, mime):
    import gi
    gi.require_version('GdkPixbuf', '2.0')
    from gi.repository import GdkPixbuf
    loader = GdkPixbuf.PixbufLoader.new_with_type(LOADERS[mime])
    invalid_size = False
    def size_prepared(loader, width, height):
        nonlocal invalid_size
        if min(width, height) <= 0 or width*height > 100_000_000:
            invalid_size = True
            loader.set_size(1, 1)
            return
        scale = min(1, MAX_SIDE / max(width, height))
        loader.set_size(max(1, round(width*scale)), max(1, round(height*scale)))
    loader.connect('size-prepared', size_prepared)
    loader.write(data)
    loader.close()
    if invalid_size:
        raise ImageConversionError('图像像素尺寸超限')
    # 明确取静态帧，不将动画/多页内容伪称为完整保留。
    animation = loader.get_animation()
    image = animation.get_static_image() if animation else loader.get_pixbuf()
    image = image.apply_embedded_orientation()
    # 部分解码器不执行 set_size，输出前再次约束尺寸。
    scale = min(1, MAX_SIDE / max(image.get_width(), image.get_height()))
    if scale < 1:
        image = image.scale_simple(max(1, round(image.get_width()*scale)),
                                   max(1, round(image.get_height()*scale)), GdkPixbuf.InterpType.BILINEAR)
    png = image.save_to_bufferv('png', ['compression'], ['6'])[1]
    if len(png) > MAX_OUTPUT:
        raise ImageConversionError('PNG 超过传输上限')
    return png


if __name__ == '__main__':
    try:
        import resource
        resource.setrlimit(resource.RLIMIT_AS, (1536*1024*1024, 1536*1024*1024))
        resource.setrlimit(resource.RLIMIT_CPU, (8, 8))
        payload = sys.stdin.buffer.read(MAX_INPUT+1)
        mime = canonical_mime(sys.argv[1])
        if len(payload) > MAX_INPUT or mime not in LOADERS or sniff_image(payload) != mime:
            raise ImageConversionError('无效输入')
        sys.stdout.buffer.write(worker(payload, mime))
    except Exception:
        sys.exit(1)

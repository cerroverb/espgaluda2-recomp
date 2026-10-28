#!/usr/bin/env python3
"""Turns futari_capture recordings into PNGs and compares two of them frame by frame.

Original code (WORKLOG.md "Original code"): futari-nx's tools/make_video.sh and compare_frames.py need ffmpeg and
its .frames hash files; this reads the raw recording (captures/<name>.rgba, R8 G8 B8 X8 per pixel, frames back to
back, size in <name>.txt: "width height fps count") directly.

  capture_compare.py png  <recording base> <out prefix>                 every frame as <out prefix>_<i>.png
  capture_compare.py diff <recording A> <recording B> <out prefix>      per-frame difference + diff images
A recording base is the path without extension, e.g. out/build/native/captures/video_5s.
"""
import sys

from PIL import Image, ImageChops


def frames(base):
    w, h, _fps, count = open(base + '.txt').read().split()
    w, h, count = int(w), int(h), int(count)
    data = open(base + '.rgba', 'rb').read()
    size = w * h * 4
    for i in range(min(count, len(data) // size)):
        yield Image.frombytes('RGBX', (w, h), data[i * size:(i + 1) * size]).convert('RGB')


def main():
    mode = sys.argv[1]
    if mode == 'png':
        for i, im in enumerate(frames(sys.argv[2])):
            im.save('%s_%d.png' % (sys.argv[3], i))
        return
    out = sys.argv[4]
    for i, (a, b) in enumerate(zip(frames(sys.argv[2]), frames(sys.argv[3]))):
        if a.size != b.size:
            print('frame %d: sizes differ %s %s' % (i, a.size, b.size))
            continue
        d = ImageChops.difference(a, b)
        hist = d.convert('L').histogram()
        total = a.size[0] * a.size[1]
        differing = total - hist[0]
        big = sum(hist[16:])
        print('frame %d: %d of %d pixels differ (%.2f%%), %d by more than 15/255 (%.2f%%), max %d' % (
            i, differing, total, 100.0 * differing / total, big, 100.0 * big / total,
            max(v for v, n in enumerate(hist) if n)))
        a.save('%s_%d_a.png' % (out, i))
        b.save('%s_%d_b.png' % (out, i))
        d.point(lambda v: min(255, v * 8)).save('%s_%d_diff.png' % (out, i))


if __name__ == '__main__':
    main()

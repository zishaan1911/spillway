"""Turns a folder of frames from `spillway --record` into a GIF.

    python tools/make_gif.py FRAMES_DIR OUT.gif [--width 640] [--fps 20]

Needs Pillow (pip install pillow).
"""

import argparse
from pathlib import Path

from PIL import Image


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("frames", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--width", type=int, default=640)
    parser.add_argument("--fps", type=int, default=20)
    args = parser.parse_args()

    paths = sorted(args.frames.glob("frame_*.png"))
    if not paths:
        raise SystemExit(f"no frame_*.png files in {args.frames}")

    frames = []
    for path in paths:
        img = Image.open(path).convert("RGB")
        height = round(img.height * args.width / img.width)
        img = img.resize((args.width, height), Image.LANCZOS)
        frames.append(img.quantize(colors=128, method=Image.Quantize.MEDIANCUT))

    frames[0].save(
        args.out,
        save_all=True,
        append_images=frames[1:],
        duration=round(1000 / args.fps),
        loop=0,
        optimize=True,
    )
    print(f"{args.out}: {len(frames)} frames, {args.out.stat().st_size / 1e6:.1f} MB")


if __name__ == "__main__":
    main()

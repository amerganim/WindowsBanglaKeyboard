#!/usr/bin/env python3
"""Builds app/res/AppIcon.ico from docs/assets/icon-source.png (the same
512x512 icon used by the Android and macOS keyboards).

Needs Pillow:  pip install pillow ;  run from the repo root:
    python scripts/make-icon.py
"""
from PIL import Image, ImageDraw

SIZES = [16, 20, 24, 32, 40, 48, 64, 128, 256]
src = Image.open("docs/assets/icon-source.png").convert("RGBA")


def rounded(size: int) -> Image.Image:
    img = src.resize((size, size), Image.LANCZOS)
    mask = Image.new("L", (size * 4, size * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, size * 4 - 1, size * 4 - 1), radius=int(size * 4 * 0.18), fill=255)
    img.putalpha(mask.resize((size, size), Image.LANCZOS))
    return img


big = rounded(256)
big.save("app/res/AppIcon.ico", sizes=[(s, s) for s in SIZES],
         append_images=[rounded(s) for s in SIZES])
big.save("docs/assets/icon-256.png")
print("wrote app/res/AppIcon.ico and docs/assets/icon-256.png")

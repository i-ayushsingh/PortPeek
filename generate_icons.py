"""
PortPeek Icon Pipeline:
- Detects the true visible logo boundaries using alpha-thresholding (> 10)
  to remove stray invisible / near-zero alpha pixels
- Places the full logo onto a square canvas with a tight ~4% breathing margin
  so the icon fills the taskbar tray cell at full size matching other apps
- Generates all standard Windows icon resolutions:
  16x16, 20x20, 24x24, 32x32, 48x48, 64x64, 128x128, 256x256
"""

import sys
from pathlib import Path
from PIL import Image, ImageFilter
import numpy as np

def generate_full_size_ico(src_path: Path, dst_path: Path, sizes=(16, 20, 24, 32, 48, 64, 128, 256)):
    with Image.open(src_path) as img:
        img = img.convert("RGBA")
        arr = np.array(img)
        alpha = arr[:, :, 3]

        # 1. Isolate visible pixels (alpha > 10) to strip ghost pixels
        mask = alpha > 10
        if not np.any(mask):
            print(f"[!] Warning: No visible pixels found in {src_path}")
            return

        y_indices, x_indices = np.where(mask)
        min_x, max_x = int(np.min(x_indices)), int(np.max(x_indices))
        min_y, max_y = int(np.min(y_indices)), int(np.max(y_indices))

        cropped = img.crop((min_x, min_y, max_x + 1, max_y + 1))
        crop_w, crop_h = cropped.size
        print(f"[*] True visible logo isolated: {crop_w}x{crop_h} (from canvas {img.size[0]}x{img.size[1]})")

        # 2. Fit to a square canvas with a tight 3% margin to maximize visual size in tray
        max_dim = max(crop_w, crop_h)
        target_canvas_size = int(max_dim / 0.94)  # Fills 94% of the icon frame

        square_img = Image.new("RGBA", (target_canvas_size, target_canvas_size), (0, 0, 0, 0))
        offset_x = (target_canvas_size - crop_w) // 2
        offset_y = (target_canvas_size - crop_h) // 2
        square_img.paste(cropped, (offset_x, offset_y), cropped)

        # 3. Multi-resolution frames with Lanczos sinc filtering
        resample_filter = getattr(Image.Resampling, 'LANCZOS', Image.LANCZOS)
        frames = []
        for s in sizes:
            dim = (s, s)
            resized = square_img.resize(dim, resample=resample_filter)

            # Micro-sharpen small sizes (<= 32px) so details pop distinctly on dark/light taskbars
            if s <= 32:
                r, g, b, a = resized.split()
                rgb = Image.merge("RGB", (r, g, b))
                sharp_rgb = rgb.filter(ImageFilter.UnsharpMask(radius=0.6, percent=140, threshold=1))
                sr, sg, sb = sharp_rgb.split()
                resized = Image.merge("RGBA", (sr, sg, sb, a))

            frames.append(resized)

        dst_path.parent.mkdir(parents=True, exist_ok=True)

        icon_sizes = [(s, s) for s in sizes]
        square_img.save(
            str(dst_path),
            format='ICO',
            sizes=icon_sizes
        )
        print(f"[+] Successfully generated: {dst_path} ({dst_path.stat().st_size:,} bytes)")

def main():
    root_dir = Path("c:/Projects/PortPeak")
    res_dir = root_dir / "res"
    appicon_src = root_dir / "appicon.png"

    if not appicon_src.exists():
        print(f"[!] {appicon_src} not found!")
        sys.exit(1)

    print("=" * 60)
    print(" Generating Full-Scale Windows Tray Icons")
    print("=" * 60)

    generate_full_size_ico(appicon_src, res_dir / "appicon.ico")
    generate_full_size_ico(appicon_src, res_dir / "icon.ico")
    print("\n[OK] Full-size icon generation completed.")

if __name__ == "__main__":
    main()

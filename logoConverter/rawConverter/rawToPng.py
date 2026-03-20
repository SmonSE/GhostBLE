from PIL import Image
import numpy as np

# --- CONFIG ---
WIDTH = 240
HEIGHT = 135
INPUT_FILE = "shot_992510_1975.raw"
OUTPUT_FILE = "output.png"

# --- LOAD RAW ---
with open(INPUT_FILE, "rb") as f:
    raw = f.read()

# Convert to numpy array (uint16)
data = np.frombuffer(raw, dtype=np.uint16)

# Convert RGB565 → RGB888
r = ((data >> 11) & 0x1F) * 255 // 31
g = ((data >> 5) & 0x3F) * 255 // 63
b = (data & 0x1F) * 255 // 31

rgb = np.stack((r, g, b), axis=-1).astype(np.uint8)

# reshape to image
rgb = rgb.reshape((HEIGHT, WIDTH, 3))

# create image
img = Image.fromarray(rgb, 'RGB')
img.save(OUTPUT_FILE)

print("Saved:", OUTPUT_FILE)
from PIL import Image
import os

def convert_png_for_m5stickcplus2(png_path, output_name="nibblesStartWorking", bmp_temp="temp.bmp"):
    # Resize to M5StickC Plus 2 resolution
    img = Image.open(png_path).convert("RGB")
    img = img.resize((240, 135))  # portrait mode

    # Save as 24-bit BMP (uncompressed)
    img.save(bmp_temp, format='BMP')

    # Read BMP binary data
    with open(bmp_temp, "rb") as f:
        bmp_data = f.read()

    # Output as C array
    print(f"const unsigned char {output_name}[] PROGMEM = {{")
    for i, b in enumerate(bmp_data):
        if i % 12 == 0:
            print("\n  ", end="")
        print(f"0x{b:02X}, ", end="")
    print("\n};")

    # Print draw macro
    print(f"\n#define BITMAP M5.Lcd.drawBmp({output_name}, sizeof({output_name}))")

    os.remove(bmp_temp)

# Usage
convert_png_for_m5stickcplus2("nibbles_working_happy_bubble.png")

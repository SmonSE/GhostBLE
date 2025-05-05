from PIL import Image
import os

def is_magentaish(r, g, b, tolerance=140):
    return abs(r - 255) < tolerance and g < tolerance and abs(b - 255) < tolerance

def convert_png_with_transparency(png_path, output_name="nibblesFront"):
    img = Image.open(png_path).convert("RGB")
    img = img.resize((230, 135))  # z. B. angry expression

    width, height = img.size
    pixels = img.load()

    print(f"const uint16_t {output_name}[] PROGMEM = {{")
    for y in range(height):
        print("  ", end="")
        for x in range(width):
            r, g, b = pixels[x, y]
            if is_magentaish(r, g, b):
                print("0xFFFF, ", end="")  # Weiß als Platzhalter für Transparent
            else:
                rgb565 = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                print(f"0x{rgb565:04X}, ", end="")
        print()
    print("};")

    print(f"\n#define {output_name.upper()}_WIDTH {width}")
    print(f"#define {output_name.upper()}_HEIGHT {height}")

# Beispielaufruf
convert_png_with_transparency("nibbles_front_happy.png", output_name="nibblesFront")

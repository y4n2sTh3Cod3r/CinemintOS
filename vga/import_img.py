from PIL import Image
import colorsys
import math
import numpy as np

# Load the image
img = Image.open("noki.png").convert("RGB")
width, height = img.size
pixels = []

# Process each pixel
for y in range(height):
    for x in range(width):
        r, g, b = img.getpixel((x, y))
        
        # Find the closest color in our palette
        rr = round(r / 51) 
        gg = round(g / 51)
        bb = round(b / 51)
        
        # Store HSV components to match your C++ struct
        pixels.append([rr, gg, bb])

# Generate header file
with open("build/sprite_item_noki.h", "w") as f:
    f.write("namespace cm {\n")
    f.write(f"const int width = {width};\n")
    f.write(f"const int height = {height};\n")
    f.write("const pixel sprite_data_noki[] = {\n")
    count = 0
    for pixel in pixels:
        f.write(f"{{{pixel[0]}, {pixel[1]}, {pixel[2]}}},")
        count += 1
        if count % width == 0:
            f.write("\n")
    f.write("};\n")
    f.write("const sprite_item sprite_item_noki = {width, height, sprite_data_noki};\n")
    f.write("}\n")
from PIL import Image
import colorsys
import math
import numpy as np
import sys
import os

# Load the image
for src in os.listdir("images"):
    print("- importing " + src)
    name = src[:src.index(".")]

    img = Image.open(os.path.join("images", src)).convert("RGB")
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
    with open("build/sprite_item_" + name + ".h", "w") as f:
        f.write("namespace sprite_" + name + " {\n")
        f.write(f"const int width = {width};\n")
        f.write(f"const int height = {height};\n")
        f.write("const cm::pixel data[] = {\n")
        count = 0
        for pixel in pixels:
            f.write(f"{{{pixel[0]}, {pixel[1]}, {pixel[2]}}},")
            count += 1
            if count % width == 0:
                f.write("\n")
        f.write("};\n")
        f.write("const cm::sprite_item item = {width, height, data};\n")
        f.write("}\n")
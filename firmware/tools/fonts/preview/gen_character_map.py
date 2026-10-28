import math
from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

# Configuration
font_path = "FontAwesome5-Solid+Brands+Regular.woff"
output_image_path = "fontawesome_char_map_labeled.png"
icon_font_size = 24     # Size of the rendered icon
cell_width = 90         # Width of each grid cell in pixels
cell_height = 55        # Height of each grid cell in pixels
cols = 14               # Number of icons per row

# Load font for icons
font = TTFont(font_path)
try:
    icon_font = ImageFont.truetype(font_path, icon_font_size)
except Exception as e:
    print(f"Error loading font into PIL: {e}")
    exit(1)

# Use Pillow's built-in default font for the text labels
label_font = ImageFont.load_default()

# Collect all unique character codes and names
glyphs = []
for table in font["cmap"].tables:
    for code, name in table.cmap.items():
        if code > 31:  # Skip control characters
            glyphs.append((code, name))

# Sort by unicode code point
glyphs = sorted(list(set(glyphs)), key=lambda x: x[0])

# Calculate image dimensions
total_glyphs = len(glyphs)
rows = math.ceil(total_glyphs / cols)
img_width = cols * cell_width
img_height = rows * cell_height

# Create a blank white image
image = Image.new("RGB", (img_width, img_height), color=(255, 255, 255))
draw = ImageDraw.Draw(image)

print(f"Rendering {total_glyphs} glyphs with labels into a {cols}x{rows} grid...")
for index, (code, name) in enumerate(glyphs):
    col = index % cols
    row = index // cols
    
    x_base = col * cell_width
    y_base = row * cell_height
    
    char = chr(code)
    hex_label = f"U+{code:04X}"
    
    # Position the icon in the upper part of the cell
    icon_x = x_base + (cell_width - icon_font_size) // 2
    icon_y = y_base + 6
    
    try:
        # Draw the icon glyph in black
        draw.text((icon_x, icon_y), char, font=icon_font, fill=(0, 0, 0))
    except Exception:
        pass
    
    # Draw the U+XXXX hex label centered near the bottom of the cell
    # Note: Using default font dimensions, approx centering or left-padding works well
    label_x = x_base + 12
    label_y = y_base + cell_height - 18
    draw.text((label_x, label_y), hex_label, font=label_font, fill=(120, 120, 120))
    
    # Draw a faint border around the cell for clean organization
    draw.rectangle([x_base, y_base, x_base + cell_width, y_base + cell_height], outline=(235, 235, 235))

# Save the final image
image.save(output_image_path)
print(f"Labeled character map successfully saved to {output_image_path}")
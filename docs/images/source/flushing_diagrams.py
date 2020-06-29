# To get the source png's of the diagram and the legend
# 1. open the svg in Inkscape
# 2. Select the items
# 3. Export to png, and export the _selection_ with 96 dpi

import glob

from PIL import Image

spacing_legend = 30


def add_padding(img, pad=25):
    new = Image.new("RGBA", (img.size[0] + 2 * pad, img.size[1] + 2 * pad), (255, 255, 255, 0))
    new.paste(img, (pad, pad))
    return new


def remove_alpha(img, color=(255, 255, 255)):
    background = Image.new("RGBA", img.size, color)
    return Image.alpha_composite(background, img)


all_images = glob.glob("flushing_diagram_*.png")
diagrams = [x for x in all_images if "legend" not in x]
legend = next(x for x in all_images if "legend" in x)

offset_x = min([Image.open(img).size[0] for img in diagrams])

lg = Image.open(legend)

for img in diagrams:
    d = Image.open(img)
    n = Image.new("RGBA", (offset_x + spacing_legend + lg.size[0], d.size[1]), (255, 255, 255, 0))
    n.paste(d, (0, 0))
    n.paste(lg, (offset_x + spacing_legend, 0))
    n = add_padding(n)
    n.save(f"../{img}", "PNG")

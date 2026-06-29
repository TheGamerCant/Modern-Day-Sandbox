from PIL import Image
import os


def main():
    for subdir, dirs, files in os.walk("C:/Users/charl/OneDrive/Documents/GitHub/Modern-Day-Sandbox/gfx/interface/technologies"):
        for file in files:
            dds_file_path: str = f"{subdir}/{file}".translate(str.maketrans("\\", "/"))

            if not dds_file_path.lower().endswith(".dds"):
                continue

            png_file_path = dds_file_path[:-3] + "png"
            
            with Image.open(dds_file_path) as img:
                img.convert("RGBA").save(png_file_path, format="PNG")
            
            os.remove(dds_file_path)

if __name__ == "__main__":
    main()
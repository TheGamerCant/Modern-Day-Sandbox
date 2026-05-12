import colorsys
import math
from PIL import Image

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def main():


    while True:
        red: float = float(input("Enter the red value: ")) / 255
        green: float = float(input("Enter the green value: ")) / 255
        blue: float = float(input("Enter the blue value: ")) / 255

        hue, saturation, value = colorsys.rgb_to_hsv(red,green,blue)

        print (hue, saturation, value)

        hue = min(round(hue * 5), 5)
        saturation = min(round(saturation * 5), 5)
        value = min(round(saturation * 4), 4)

        print (hue, saturation, value)


if __name__ == "__main__":
    main()
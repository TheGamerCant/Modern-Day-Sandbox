from PIL import Image
import os
import shutil
import re
import numpy as np
from multiprocessing import Pool

ideologyArray = np.array(
        ["IDEOLOGY_fascist", "IDEOLOGY_ultranationalist", "IDEOLOGY_right_despotist", "IDEOLOGY_fundamentalist", "IDEOLOGY_monarchist", "IDEOLOGY_populist",
        "IDEOLOGY_libertarian", "IDEOLOGY_conservative", "IDEOLOGY_liberal", "IDEOLOGY_social_democrat",
        "IDEOLOGY_socialist", "IDEOLOGY_moderate_communist", "IDEOLOGY_hardline_communist", "IDEOLOGY_left_despotist", "IDEOLOGY_anarchist"])


def makeFlags(info: str) -> str:
    split_text = info.split("--")
    tag = split_text[0]
    modDirectory = split_text[1]
    
    flagPathString = os.path.join(modDirectory, "__code", "countryMaker", "flags", tag + ".png")
    if os.path.exists(flagPathString):
        flagLarge = Image.open(flagPathString).convert('RGBA')

        flagLarge = flagLarge.resize((82, 52))
        flagMedium = flagLarge.resize((41, 26))
        flagSmall = flagLarge.resize((10, 7))

        for ideology in ideologyArray:
            ideologyFlagNameStringTGA = tag + "_" + ideology + ".tga"
            ideologyFlagNameStringPNG = tag + "_" + ideology + ".png"
            ideologyFlagPathString = os.path.join(modDirectory, "__code", "countryMaker", "flags", ideologyFlagNameStringPNG)

            if os.path.isfile(ideologyFlagPathString):
                ideologyFlagLarge = Image.open(ideologyFlagPathString).convert('RGBA')
                ideologyFlagLarge = ideologyFlagLarge.resize((82, 52))
                ideologyFlagMedium = ideologyFlagLarge.resize((41, 26))
                ideologyFlagSmall = ideologyFlagLarge.resize((10, 7))

                ideologyFlagLarge.save(os.path.join(modDirectory, "gfx", "flags", ideologyFlagNameStringTGA), format='TGA', rle=False)
                ideologyFlagMedium.save(os.path.join(modDirectory, "gfx", "flags", "medium", ideologyFlagNameStringTGA), format='TGA', rle=False)
                ideologyFlagSmall.save(os.path.join(modDirectory, "gfx", "flags", "small", ideologyFlagNameStringTGA), format='TGA', rle=False)

                ideologyFlagLarge.close()
                ideologyFlagMedium.close()
                ideologyFlagSmall.close()

            else:
                flagLarge.save(os.path.join(modDirectory, "gfx", "flags", ideologyFlagNameStringTGA), format='TGA', rle=False)
                flagMedium.save(os.path.join(modDirectory, "gfx", "flags", "medium", ideologyFlagNameStringTGA), format='TGA', rle=False)
                flagSmall.save(os.path.join(modDirectory, "gfx", "flags", "small", ideologyFlagNameStringTGA), format='TGA', rle=False)

        flagLarge.close()
        flagMedium.close()
        flagSmall.close()
        return "Tag " + tag + " has successfully had flags created."
    
    return "Tag " + tag + " failed to create flags"

def loadCosmeticTags(directory: str) -> np.array:
    tags = []
    if os.path.exists(directory):
        with open(directory, 'r') as file:
            content = file.read()

        tags = re.findall(r'(\w+)\s*=\s*\{', content)

    tagsArray = np.array(tags, dtype='U')
    return tagsArray

def flagsMain(modDirectory: str, countryTagsArray):

    gfxFlagsLargeDirectory = os.path.join(modDirectory, "gfx", "flags")
    gfxFlagsMediumDirectory = os.path.join(gfxFlagsLargeDirectory, "medium")
    gfxFlagsSmallDirectory = os.path.join(gfxFlagsLargeDirectory, "small")

    # Delete gfx/flags and make new directories
    shutil.rmtree(gfxFlagsLargeDirectory, ignore_errors=True)
    os.makedirs(gfxFlagsMediumDirectory)
    os.makedirs(gfxFlagsSmallDirectory)

    # Load dynamic tags
    cosmeticTagsArray = loadCosmeticTags(os.path.join(modDirectory, "common", "countries", "cosmetic.txt"))

    countryTagsArray = np.char.add(countryTagsArray, "--" + modDirectory)

    # Create flags using multiprocessing
    with Pool() as pool:
        results = pool.imap_unordered(makeFlags, countryTagsArray)
        for tag in results:
            print(tag)

    if cosmeticTagsArray.size != 0:
        cosmeticTagsArray = np.char.add(cosmeticTagsArray, "--" + modDirectory)
        with Pool() as pool:
            results = pool.imap_unordered(makeFlags, cosmeticTagsArray)
            for tag in results:
                print(tag)
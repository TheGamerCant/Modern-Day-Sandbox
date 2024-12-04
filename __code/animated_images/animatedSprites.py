from PIL import Image
import os
import shutil
import math
import time

videoFramesPerSecond = 20.0
videoOutNamePrefix = "test_frame_"

animImagesFolder = "gfx/interface/anim/"    
animImagesLooping = "yes"

def outputFiles(noOfFiles: int):
    rBracket = "{"
    lBracket = "}"
    with open("out/GFX.gfx", "w") as f:
        print(f"spriteTypes = {rBracket}", file=f)

        for i in range(noOfFiles):
            print(f"\tFrameAnimatedSpriteType = {rBracket}\
                  \n\t\tname = \"GFX_{videoOutNamePrefix}{i}\
                  \n\t\ttextureFile = \"{animImagesFolder}{videoOutNamePrefix}{i}.png\"\
                  \n\t\tnoOfFrames = {videoNoOfFrames}\
                  \n\t\talwaysTransparent = yes\
                  \n\t\tlooping = {animImagesLooping}\
                  \n\t\tplay_on_show = yes\
                  \n\t\tpause_on_loop = 0.0\
                  \n\t{lBracket}", file=f)
            
        print(f"{lBracket}", file=f)

def getImageData(directories: list[str]):
    global videoWidth, videoHeight, videoNoOfFrames

    videoNoOfFrames = len(directories)
    img = Image.open(directories[0])
    videoWidth, videoHeight = img.size
    img.close()

def createBlankImages(sliceSizeArray: list[int]):
    shutil.rmtree("out", ignore_errors=True)
    os.makedirs("out")
    for index, pxWidth in enumerate(sliceSizeArray):
        outName = videoOutNamePrefix + str(index) + ".png"
        thisWidth = pxWidth * videoNoOfFrames
        image = Image.new("RGBA", (thisWidth, videoHeight), (255,0,255,255))
        image.save(outName)
        image.close()

def outputImages(sliceSizeArray: list[int], directories: list[str]):
    xPos = 0
    for outputIndex, pxWidth in enumerate(sliceSizeArray):
        outName = videoOutNamePrefix + str(outputIndex) + ".png"
        imgArray = [None for i in range(len(directories))]
        for inputIndex, path in enumerate(directories):
            imgArray[inputIndex] = Image.open(path)
            imgArray[inputIndex] = imgArray[inputIndex].crop((xPos, 0, (xPos+pxWidth), videoHeight))
        
        currentOutImg = Image.open(outName)

        for index, img in enumerate(imgArray):
            currentOutImg.paste(img, ((index * pxWidth),0))
            img.close()

        currentOutImg.save(outName)
        currentOutImg.close()
        xPos+= pxWidth


def returnNoOfOutImages():
    noOfOutImages = float( videoWidth * videoHeight * videoNoOfFrames )
    noOfOutImages = int(math.ceil(noOfOutImages / 5000000))

    quotient = videoWidth // noOfOutImages
    remainder = videoHeight % noOfOutImages

    sliceSizeArray = [quotient] * noOfOutImages
    for i in range(remainder):
        sliceSizeArray[i] += 1

    return sliceSizeArray

def main():
    start_t = time.perf_counter()

    global videoOutNamePrefix
    videoOutNamePrefix = "out/" + videoOutNamePrefix

    directories = os.listdir('in')
    directories = ["in/" + entry for entry in directories if entry.endswith(".png")]

    getImageData(directories)

    sliceSizeArray = returnNoOfOutImages()
    createBlankImages(sliceSizeArray)
    outputImages (sliceSizeArray, directories)

    outputFiles(len(sliceSizeArray))

    end_t = time.perf_counter()
    total_duration = end_t - start_t
    print (f"Took {total_duration:.2f}s")

videoWidth = None
videoHeight = None
videoNoOfFrames = None

if __name__ == "__main__":
    main()

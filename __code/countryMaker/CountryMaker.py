import os
import tkinter as tk
from tkinter import ttk
import numpy as np
import re
from writeFlags import flagsMain

#Defined in common/graphicalculturetype.txt
culturesArray = (
    "western_european","eastern_european","asian","southamerican","middle_eastern","african"
)

ideologiesArray = (
    "IDEOLOGY_fascist", "IDEOLOGY_ultranationalist", "IDEOLOGY_right_despotist", "IDEOLOGY_fundamentalist",  "IDEOLOGY_monarchist", "IDEOLOGY_populist", "IDEOLOGY_libertarian", "IDEOLOGY_conservative", "IDEOLOGY_liberal", "IDEOLOGY_social_democrat", "IDEOLOGY_socialist", "IDEOLOGY_moderate_communist", "IDEOLOGY_hardline_communist", "IDEOLOGY_left_despotist", "IDEOLOGY_anarchist"
)

def isValidTag(tag: str) -> bool:
#    regexPattern = r'^[A-Z][A-Z0-9]{2}$'       #Can technically have digits but that's bad practice, generally only used for dynamic tags
    regexPattern = r'^[A-Z]{3}$'
    return re.match(regexPattern, tag)

class countryMakerWindow:
    def __init__(self, root, modDirectory, countryTagsArray):
        self.root = root
        self.root.title("It's MY code shitass, and you're a FAGGOT!")

        self.modDirectory = modDirectory
        self.countryTagsArray = countryTagsArray

        #Tag Entry
        self.tagEntryText = tk.Label(root, text="Enter a tag")
        self.tagEntryText.grid(row=0, column=0, padx=10, pady=10)
        self.tagEntryInput = tk.Entry(root, width=40)
        self.tagEntryInput.grid(row=0, column=1, padx=10, pady=10)
        self.tagEntryInput.insert (0, "USA")

        #Localisation
        self.tagLocText = tk.Label(root, text="Country Name")
        self.tagLocText.grid(row=1, column=0, padx=10, pady=10)
        self.tagLocInput = tk.Entry(root, width=40)
        self.tagLocInput.grid(row=1, column=1, padx=10, pady=10)
        self.tagLocInput.insert (0, "United States of America")

        self.tagDEFText = tk.Label(root, text="Country DEF")
        self.tagDEFText.grid(row=2, column=0, padx=10, pady=10)
        self.tagDEFInput = tk.Entry(root, width=40)
        self.tagDEFInput.grid(row=2, column=1, padx=10, pady=10)
        self.tagDEFInput.insert (0, "the United States of America")

        self.tagADJText = tk.Label(root, text="Country ADJ")
        self.tagADJText.grid(row=3, column=0, padx=10, pady=10)
        self.tagADJInput = tk.Entry(root, width=40)
        self.tagADJInput.grid(row=3, column=1, padx=10, pady=10)
        self.tagADJInput.insert (0, "American")

                #Capital State
        self.capitalStateText = tk.Label(root, text="Capital")
        self.capitalStateText.grid(row=4, column=0, padx=10, pady=10)
        self.capitalStateInput = tk.Entry(root, width=40)
        self.capitalStateInput.grid(row=4, column=1, padx=10, pady=10)
        self.capitalStateInput.insert (0, "1")
        
        #Culture Combobox
        self.graphicalCultureText = tk.Label(root, text="Graphical Culture")
        self.graphicalCultureText.grid(row=5, column=0, padx=10, pady=10)
        self.graphicalCultureStr = tk.StringVar()
        self.graphicalCultureDropdownBox = ttk.Combobox(root, textvariable=self.graphicalCultureStr)
        self.graphicalCultureDropdownBox['values'] = culturesArray
        self.graphicalCultureDropdownBox.grid(row=5, column=1, padx=10, pady=10)
        self.graphicalCultureDropdownBox.current(0)  # Set default selection

        #Ruling Party Combobox
        self.RulingPartyText = tk.Label(root, text="Ruling Party")
        self.RulingPartyText.grid(row=6, column=0, padx=10, pady=10)
        self.RulingPartyStr = tk.StringVar()
        self.RulingPartyDropdownBox = ttk.Combobox(root, textvariable=self.RulingPartyStr)
        self.RulingPartyDropdownBox['values'] = ideologiesArray
        self.RulingPartyDropdownBox.grid(row=6, column=1, padx=10, pady=10)
        self.RulingPartyDropdownBox.current(0)  # Set default selection

        #RGB entry
        self.rgbText = tk.Label(root, text="RGB:")
        self.rgbText.grid(row=7, column=0, padx=10, pady=10)
        self.rSlider = tk.Scale(root, from_=0, to=255, orient=tk.HORIZONTAL, length=280)
        self.rSlider.grid(row=7, column=1, padx=10, pady=10)
        self.gSlider = tk.Scale(root, from_=0, to=255, orient=tk.HORIZONTAL, length=280)
        self.gSlider.grid(row=8, column=1, padx=10, pady=10)
        self.bSlider = tk.Scale(root, from_=0, to=255, orient=tk.HORIZONTAL, length=280)
        self.bSlider.grid(row=9, column=1, padx=10, pady=10)
        
        #Create Country Button
        self.countryButton = tk.Button(root, text="Create Country", command = self.createCountry)
        self.countryButton.grid(row=11, column=0, columnspan=1, padx=10, pady=10)

        #Update Flags Button
        self.flagsButton = tk.Button(root, text="Update Flags", command = lambda : flagsMain(self.modDirectory, self.countryTagsArray))
        self.flagsButton.grid(row=11, column=1, columnspan=1, padx=10, pady=10)

        
    def createCountry(self):
        tag = self.tagEntryInput.get()
        if (not(tag in self.countryTagsArray) and isValidTag(tag)):
            red, green, blue = self.rSlider.get(), self.gSlider.get(), self.bSlider.get()
            countryLocalisation = self.tagLocInput.get()
            countryLocalisationDEF = self.tagDEFInput.get()
            countryLocalisationADJ = self.tagADJInput.get()
            culture = self.graphicalCultureStr.get()
            rulingParty = self.RulingPartyStr.get()
            capitalState = self.capitalStateInput.get()
            countryStringNoSpaces = re.sub(r'\s+', '_', countryLocalisation)
#            print (tag, red, green, blue, countryLocalisation, countryLocalisationDEF, countryLocalisationADJ, culture)

            countryLocalisationFile = os.path.join(self.modDirectory, "localisation", "english", "countries_l_english.yml")
            with open(countryLocalisationFile, 'a') as locFile:
                locFile.write(f"\n\n {tag}:0 \"{countryLocalisation}\"\
                                  \n {tag}_DEF:0 \"{countryLocalisationDEF}\"\
                                  \n {tag}_ADJ:0 \"{countryLocalisationADJ}\"")

            countryHistoryFileName = tag + " - " + countryLocalisation + ".txt"
            countryHistoryFile = os.path.join(self.modDirectory, "history", "countries", countryHistoryFileName)
            with open(countryHistoryFile, 'w') as historyFile:
                historyFile.write(f"capital = {capitalState}\n\nset_popularities = {{\n\t{rulingParty} = 100\n}}\n\nset_politics = {{\
                                  \n\truling_party = {rulingParty}\n\tlast_election = \"2021.1.1\"\n\telection_frequency = 48\
                                  \n\telections_allowed = no\n}}")

            countryTagsFile = os.path.join(self.modDirectory, "common", "country_tags", "00_countries.txt")
            with open(countryTagsFile, 'a') as tagsFile: 
                tagsFile.write(f"\n{tag} = \"countries/{tag}_{countryStringNoSpaces}.txt\"")

            countryTagFileName = tag + "_" + countryStringNoSpaces + ".txt"
            countryTagFile = os.path.join(self.modDirectory, "common", "countries", countryTagFileName)
            with open(countryTagFile, 'w') as tagFile:
                rgbString = "{ " + str(red) + " " + str(green) + " " + str(blue) + " }"
                tagFile.write(f"\n\n\ngraphical_culture = {culture}_gfx\ngraphical_culture_2d = {culture}_2d\n\ncolor = rgb {rgbString}")

            countryColoursFile = os.path.join(self.modDirectory, "common", "countries", "colors.txt")
            with open(countryColoursFile, 'a') as colourFile:
                rgbString = "{ " + str(red) + " " + str(green) + " " + str(blue) + " }"
                outStringA = tag + " = {"
                outStringB = "}"
                colourFile.write(f"\n{outStringA}\n\tcolor = rgb {rgbString}\n\tcolor_ui = rgb {rgbString}\n{outStringB}")

            self.countryTagsArray = np.sort(np.append(self.countryTagsArray, tag))

        else:
            print ("Invalid Tag")


def loadCountryTags(directory: str) -> np.array:
    tagsArray = []

    for file_name in os.listdir(directory):
        if file_name.endswith('.txt'):
            file_path = os.path.join(directory, file_name)
            with open(file_path, 'r') as file:
                for line in file:           #We have to go line-by-line incase of dynamic declaration
                    isDynamic = re.search("dynamic_tags\\s*=\\s*yes", line.strip())
                    if isDynamic:
                        break
                    tagRegex = re.search("(\\w{3})\\s*=\\s*\".*\"", line.strip())
                    if tagRegex:
                        tagsArray.append(tagRegex.group(1))

    countryTagsArray = np.array(tagsArray, dtype='U')
    return countryTagsArray

def main():
    currentDirectory = os.getcwd()
    modDirectory = os.path.dirname(os.path.dirname(currentDirectory))
    countryTagsArray = np.sort(loadCountryTags(os.path.join(modDirectory, "common", "country_tags")))

    root = tk.Tk()
    app = countryMakerWindow(root, modDirectory, countryTagsArray)

    root.mainloop()

if __name__ == "__main__":
    main()
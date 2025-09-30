import json
import os
from typing import Union

#Define our output directories
OUTPUT_DIR = "out"
IDEAS_FILE = os.path.join(OUTPUT_DIR, "ideas.txt")
EFFECTS_FILE = os.path.join(OUTPUT_DIR, "scripted_effects.txt")

#Define our transformation class
class transformationClass:
    def __init__(self, transformation_id : int, transformation_name : str, prerequisite : Union[list[list[int]], None],
                 mutually_exclusive : Union[list[int], None], modifiers : Union[dict[str, str], None],
                 equipment_modifiers : Union[dict[str, dict[str, str]], None],
                 research_bonuses : Union[dict[str, str], None], targeted_modifiers : Union[dict[str, dict[str, str]], None],
                 rules : Union[dict[str, str], None]):
        
        self.transformation_id : int = transformation_id
        self.transformation_name : str = transformation_name
        self.prerequisite : Union[list[list[int]], None] = prerequisite
        self.mutually_exclusive : Union[list[int], None] = mutually_exclusive
        self.modifiers : Union[dict[str, str], None] = modifiers
        self.equipment_modifiers : Union[dict[str, dict[str, str]], None] = equipment_modifiers
        self.research_bonuses : Union[dict[str, str], None] = research_bonuses
        self.targeted_modifiers : Union[dict[str, dict[str, str]], None] = targeted_modifiers
        self.rules : Union[dict[str, str], None] = rules

#Define our idea class
class ideaClass:
    def __init__(self, idea_id : str, localised_name : str, gfx : Union[str, None], modifiers : Union[dict[str, str], None],
                 equipment_modifiers : Union[dict[str, dict[str, str]], None], research_bonuses : Union[dict[str, str], None],
                 targeted_modifiers : Union[dict[str, dict[str, str]], None], rules : Union[dict[str, str], None]):
        self.idea_id : str = idea_id
        self.localised_name : str = localised_name
        self.gfx : str = gfx

        self.modifiers : dict[str, str] = modifiers
        self.equipment_modifiers : dict[str, dict[str, str]] = equipment_modifiers
        self.research_bonuses : dict[str, str] = research_bonuses
        self.targeted_modifiers : dict[str, dict[str, str]] = targeted_modifiers
        self.rules : dict[str, str] = rules

#Get a dictionary for transformation names -> index
def getTransformationsIndexDictionary(transformationsJson) -> dict[str, int]:
    transformationsNamesArray : list[str] = []
    for transformation in transformationsJson:
        transformation_name = transformation.get("transformation_name", None)
        if transformation_name is None:
            raise Exception("Every transformation needs a name!")
    
        if transformation_name in transformationsNamesArray:
            raise Exception(transformation_name, " appears twice - no two transformations can have the same name!")
        
        transformationsNamesArray.append(transformation_name)
    
    return {name: index for index, name in enumerate(transformationsNamesArray)}

#Get a simple array of all transformations
def getTransformationsArray(transformationsJson, transformationsIndexDict : dict[str, int]) -> list[transformationClass]:
    transformationsArray : list[transformationClass] = []
    for index, transformation in enumerate(transformationsJson):
        transformation_name : str = transformation.get("transformation_name", None)
        prerequisite = transformation.get("prerequisite", None)

        #Check if the list is a 2d string array and handle indexes[list[str]] by turning them into a 2d array of indexes
        if all(isinstance(row, list) and all(isinstance(item, str) for item in row) for row in prerequisite):
            prerequisite = [[transformationsIndexDict.get(trnsfrm_name) for trnsfrm_name in entry] for entry in prerequisite]
        #If not 2d array or None raise exception
        elif prerequisite is not None:
            raise Exception("The prerequisites entry for transformation", transformation_name,
                            "needs to be a two-dimensional array!")


        mutually_exclusive = transformation.get("mutually_exclusive", None)
        #Make sure
        if isinstance(mutually_exclusive, list) and all(isinstance(item, str) for item in mutually_exclusive):
            mutually_exclusive = [transformationsIndexDict.get(trnsfrm_name) for trnsfrm_name in mutually_exclusive]
        elif mutually_exclusive is not None:
            raise Exception("The mutually_exclusive entry for", transformation_name,
                            "needs to be an array!")

        modifiers : Union[dict[str, str], None] = transformation.get("modifiers", None)
        equipment_modifiers : Union[dict[str, dict[str, str]], None] = transformation.get("equipment_modifiers", None)
        research_bonuses : Union[dict[str, str], None] = transformation.get("research_bonuses", None)
        targeted_modifiers : Union[dict[str, dict[str, str]] , None]= transformation.get("targeted_modifiers", None)
        rules : Union[dict[str, str], None] = transformation.get("rules", None)

        transformationsArray.append(transformationClass(
            index, transformation_name, prerequisite, mutually_exclusive, modifiers,
            equipment_modifiers, research_bonuses, targeted_modifiers, rules
        ))
    
    return transformationsArray

#Validate data in the transformations array - for now, we are just checking mutually exclusives
def validateTransformationsArray(transformationsArray : list[transformationClass]):
    for transformaiton in transformationsArray:
        this_id : int = transformaiton.transformation_id
        
        for mutual_transformaition in transformaiton.mutually_exclusive:
            if not this_id in transformationsArray[mutual_transformaition].mutually_exclusive:
                raise Exception("Transformation", transformationsArray[this_id].transformation_name,
                                "contains a mutually_exclusive argument for transformation", transformationsArray[mutual_transformaition].transformation_name,
                                "which does not contain a mutully_exclusive argument back!")


#Idea handling
def parse_idea(ideasArray : list, jsonData):
    for idea in jsonData.get("ideas", None):
        idea_id : str = idea.get("idea_id", None)
        localised_name : str = idea.get("localised_name", "")
        gfx : Union[str, None] = idea.get("gfx", None)

        if idea_id is None or idea_id == "":
            raise Exception("Idea ID cannot be unset!")
        
        base_modifiers = idea.get("base_modifiers")

        modifiers : Union[dict[str, str], None] = base_modifiers.get("modifiers", None)
        equipment_modifiers : Union[dict[str, dict[str, str]], None] = base_modifiers.get("equipment_modifiers", None)
        research_bonuses : Union[dict[str, str], None] = base_modifiers.get("research_bonuses", None)
        targeted_modifiers : Union[dict[str, dict[str, str]], None] = base_modifiers.get("targeted_modifiers", None)
        rules : Union[dict[str, str], None] = base_modifiers.get("rules", None)

        ideasArray.append(ideaClass(
            idea_id, localised_name, gfx, modifiers, equipment_modifiers, research_bonuses, targeted_modifiers, rules
        ))

        #We now have our base idea - get the transformations
        transformationsJson = idea.get("transformations", None)
        if transformationsJson is None:
            raise Exception("The idea", idea_id, " has no transformations!")
        
        transformationsIndexDict : dict[str, int] = getTransformationsIndexDictionary(transformationsJson)
        transformationsArray : list[transformationClass] = getTransformationsArray(transformationsJson, transformationsIndexDict)

        #We now have our transformations array - start by validating the data
        validateTransformationsArray(transformationsArray)

        print("")

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open("ideas.json", "r", encoding="utf-8") as json_file:
        jsonData = json.load(json_file)

    ideasArray = []
    parse_idea(ideasArray, jsonData)

    
        


if __name__ == "__main__":
    main()
import json
import os

#Define our output directories
OUTPUT_DIR = "out"
IDEAS_FILE = os.path.join(OUTPUT_DIR, "ideas.txt")
EFFECTS_FILE = os.path.join(OUTPUT_DIR, "scripted_effects.txt")

#Define our transformation classes
class transformationClass:
    def __init__(self, transformation_id : int, transformation_name : str, prerequisite : list, mutually_exclusive : list[int],
                 modifiers : dict[str, str], equipment_modifiers : dict[str, dict[str, str]], research_bonuses : dict[str, str],
                 targeted_modifiers : dict[str, dict[str, str]], rules : dict[str, str]):
        
        self.transformation_id : int = transformation_id
        self.transformation_name : str = transformation_name
        self.prerequisite : list = prerequisite
        self.mutually_exclusive : list[int] = mutually_exclusive
        self.modifiers : dict[str, str] = modifiers
        self.equipment_modifiers : dict[str, dict[str, str]] = equipment_modifiers
        self.research_bonuses : dict[str, str] = research_bonuses
        self.targeted_modifiers : dict[str, dict[str, str]] = targeted_modifiers
        self.rules : dict[str, str] = rules

#Define our idea class
class ideaClass:
    def __init__(self, idea_id : str, localised_name : str, gfx : str, modifiers : dict[str, str],
                 equipment_modifiers : dict[str, dict[str, str]], research_bonuses : dict[str, str],
                 targeted_modifiers : dict[str, dict[str, str]], rules : dict[str, str]):
        self.idea_id : str = idea_id
        self.localised_name : str = localised_name
        self.gfx : str = gfx

        self.modifiers : dict[str, str] = modifiers
        self.equipment_modifiers : dict[str, dict[str, str]] = equipment_modifiers
        self.research_bonuses : dict[str, str] = research_bonuses
        self.targeted_modifiers : dict[str, dict[str, str]] = targeted_modifiers
        self.rules : dict[str, str] = rules

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

def getTransformationsArray(transformationsJson, transformationsIndexDict : dict[str, int]) -> list:
    transformationsArray = []
    for transformation in transformationsJson:
        transformation_name : str = transformation.get("name", None)
        prerequisite = transformation.get("prerequisite", [])
        mutually_exclusive = transformation.get("mutually_exclusive", [])
        modifiers : dict[str, str] = transformation.get("modifiers", None)
        equipment_modifiers : dict[str, dict[str, str]] = transformation.get("equipment_modifiers", None)
        research_bonuses : dict[str, str] = transformation.get("research_bonuses", None)
        targeted_modifiers : dict[str, dict[str, str]] = transformation.get("targeted_modifiers", None)
        rules : dict[str, str] = transformation.get("rules", None)


def parse_idea(ideasArray : list, jsonData):
    for idea in jsonData.get("ideas", None):
        idea_id : str = idea.get("idea_id", None)
        localised_name : str = idea.get("localised_name", "")
        gfx : str = idea.get("gfx", None)

        if idea_id is None:
            raise Exception("Idea ID cannot be unset!")
        
        base_modifiers = idea.get("base_modifiers")

        modifiers : dict[str, str] = base_modifiers.get("modifiers", None)
        equipment_modifiers : dict[str, dict[str, str]] = base_modifiers.get("equipment_modifiers", None)
        research_bonuses : dict[str, str] = base_modifiers.get("research_bonuses", None)
        targeted_modifiers : dict[str, dict[str, str]] = base_modifiers.get("targeted_modifiers", None)
        rules : dict[str, str] = base_modifiers.get("rules", None)

        ideasArray.append(ideaClass(idea_id, localised_name, gfx, modifiers, equipment_modifiers, research_bonuses, targeted_modifiers, rules))

        #We now have our base idea - get the transformations
        transformationsJson = idea.get("transformations", None)
        if transformationsJson is None:
            raise Exception("The idea", idea_id, " has no transformations!")
        
        transformationsIndexDict : dict[str, int] = getTransformationsIndexDictionary(transformationsJson)
        transformationsArray : list = getTransformationsArray(transformationsJson, transformationsIndexDict)

        print("")
        print(transformationsArray)

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open("ideas.json", "r", encoding="utf-8") as json_file:
        jsonData = json.load(json_file)

    ideasArray = []
    parse_idea(ideasArray, jsonData)

    
        


if __name__ == "__main__":
    main()
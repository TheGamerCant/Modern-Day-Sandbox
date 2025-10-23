import json
import os
from typing import Union
from decimal import Decimal, InvalidOperation
import copy
import math

#Define our output directories
OUTPUT_DIR = "out"
IDEAS_FILE = os.path.join(OUTPUT_DIR, "ideas.txt")
EFFECTS_FILE = os.path.join(OUTPUT_DIR, "scripted_effects.txt")
LOCALISATION_FILE = os.path.join(OUTPUT_DIR, "ideas_l_english.yml")
USE_INDEX_VARIABLE = False

#Check if a string can be converted into a decimal
def boolStringIsDecimal(s: str) -> bool:
    try:
        Decimal(s)
        return True
    except (InvalidOperation, ValueError):
        return False

#Define our transformation class
class transformationClass:
    def __init__(self, transformation_index : int, transformation_id : str, localised_name : str, localised_desc : str,
                 prerequisite : Union[list[list[int]], None], mutually_exclusive : Union[list[int], None], 
                 modifiers : Union[dict[str, str], None], equipment_modifiers : Union[dict[str, dict[str, str]], None],
                 research_bonuses : Union[dict[str, str], None], targeted_modifiers : Union[dict[str, dict[str, str]], None],
                 rules : Union[dict[str, str], None]):
        
        self.transformation_index : int = transformation_index
        self.transformation_id : str = transformation_id
        self.localised_name : str = localised_name
        self.localised_desc : str = localised_desc
        self.prerequisite : Union[list[list[int]], None] = prerequisite
        self.mutually_exclusive : Union[list[int], None] = mutually_exclusive
        self.modifiers : Union[dict[str, str], None] = modifiers
        self.equipment_modifiers : Union[dict[str, dict[str, str]], None] = equipment_modifiers
        self.research_bonuses : Union[dict[str, str], None] = research_bonuses
        self.targeted_modifiers : Union[dict[str, dict[str, str]], None] = targeted_modifiers
        self.rules : Union[dict[str, str], None] = rules

#Define our idea class
class ideaClass:
    def __init__(self, idea_id : str, localised_name : str, localised_desc : str, gfx : str, modifiers : Union[dict[str, str], None],
                 equipment_modifiers : Union[dict[str, dict[str, str]], None], research_bonuses : Union[dict[str, str], None],
                 targeted_modifiers : Union[dict[str, dict[str, str]], None], rules : Union[dict[str, str], None]):
        self.idea_id : str = idea_id
        self.localised_name : str = localised_name
        self.localised_desc : str = localised_desc
        self.gfx : str = gfx
        self.idea_routes : list[list[int]] = [[]]

        self.modifiers : dict[str, str] = modifiers
        self.equipment_modifiers : dict[str, dict[str, str]] = equipment_modifiers
        self.research_bonuses : dict[str, str] = research_bonuses
        self.targeted_modifiers : dict[str, dict[str, str]] = targeted_modifiers
        self.rules : dict[str, str] = rules

    def applyTransformation(self, transformation : transformationClass):
        if transformation.modifiers is not None:
            for key, value in transformation.modifiers.items():
                if not boolStringIsDecimal(value):
                    raise Exception("Modifier value for", key, "in transformation",
                        transformation.transformation_id, "is not a decimal value!")
                self.modifiers[key] = str(Decimal(self.modifiers.get(key, "0.00")) + Decimal(value))
        
        if transformation.equipment_modifiers is not None:
            for equipment_key, equipment_modifiers_dictionary in transformation.equipment_modifiers.items():
                #If this equipment type doesn't already exist just add the modifier dictionary as-is
                if self.equipment_modifiers.get(equipment_key, None) == None:
                    self.equipment_modifiers[equipment_key] = equipment_modifiers_dictionary
                else:
                    for key, value in equipment_modifiers_dictionary.items():
                        #Need unique handling for instant = yes/no
                        if key == "instant":
                            self.equipment_modifiers[equipment_key][key] = value
                        else:
                            if not boolStringIsDecimal(value):
                                raise Exception("Modifier value for",equipment_key, "->", key, "in transformation",
                                    transformation.transformation_id, "is not a decimal value!")
                            self.equipment_modifiers[equipment_key][key] = str(Decimal(self.equipment_modifiers[equipment_key].get(key, "0.00")) + Decimal(value))

        if transformation.research_bonuses is not None:
            for key, value in transformation.research_bonuses.items():
                if not boolStringIsDecimal(value):
                    raise Exception("Research bonus value for", key, "in transformation",
                        transformation.transformation_id, "is not a decimal value!")
                self.research_bonuses[key] = str(Decimal(self.research_bonuses.get(key, "0.00")) + Decimal(value))

        if transformation.targeted_modifiers is not None:
            for target_tag, targeted_modifiers_dictionary in transformation.targeted_modifiers.items():
                if self.targeted_modifiers.get(target_tag, None) == None:
                    self.targeted_modifiers[target_tag] = targeted_modifiers_dictionary
                else:
                    for key, value in targeted_modifiers_dictionary.items():
                        if not boolStringIsDecimal(value):
                            raise Exception("Targeted modifier value for", key, "against", target_tag, "in transformation",
                                transformation.transformation_id, "is not a decimal value!")
                        self.targeted_modifiers[target_tag][key] = str(Decimal(self.targeted_modifiers[target_tag].get(key, "0.00")) + Decimal(value))

        if transformation.rules is not None:
            for key, value in transformation.rules.items():
                self.rules[key] = value
   
        if transformation.localised_name != "" and transformation.localised_name is not None:
            self.localised_name = transformation.localised_name

        if transformation.localised_desc != "" and transformation.localised_desc is not None:
            self.localised_desc = transformation.localised_desc


    def append_int_to_idea_route(self, node : int):
        self.idea_route.append(node)

def boolAreTwoIdeasEqual(idea_one : ideaClass, idea_two : ideaClass):
    if idea_one.modifiers == idea_two.modifiers and idea_one.equipment_modifiers == idea_two.equipment_modifiers \
    and idea_one.research_bonuses == idea_two.research_bonuses \
    and idea_one.targeted_modifiers == idea_two.targeted_modifiers and idea_one.rules == idea_two.rules \
    and idea_one.localised_name == idea_two.localised_name and idea_one.localised_desc == idea_two.localised_desc:
        return True
    
    return False

#Get a dictionary for transformation names -> index
def getTransformationsIndexDictionary(transformationsJson) -> dict[str, int]:
    transformationsNamesArray : list[str] = []
    for transformation in transformationsJson:
        transformation_id = transformation.get("id", None)
        if transformation_id is None:
            raise Exception("Every transformation needs a name!")
    
        if transformation_id in transformationsNamesArray:
            raise Exception(transformation_id, " appears twice - no two transformations can have the same name!")
        
        transformationsNamesArray.append(transformation_id)
    
    return {name: index for index, name in enumerate(transformationsNamesArray)}

#Get a simple array of all transformations
def getTransformationsArray(transformationsJson, transformationsIndexDict : dict[str, int]) -> list[transformationClass]:
    transformationsArray : list[transformationClass] = []
    for index, transformation in enumerate(transformationsJson):
        transformation_id : str = transformation.get("id", None)
        prerequisite = transformation.get("prerequisite", None)

        localised_name : str = transformation.get("localised_name", "")
        localised_desc : str = transformation.get("localised_desc", "")

        #Check if the list is a 2d string array and handle indexes[list[str]] by turning them into a 2d array of indexes
        if all(isinstance(row, list) and all(isinstance(item, str) for item in row) for row in prerequisite):
            prerequisite = [[transformationsIndexDict.get(trnsfrm_name) for trnsfrm_name in entry] for entry in prerequisite]
        #If not 2d array or None raise exception
        elif prerequisite is not None:
            raise Exception("The prerequisites entry for transformation", transformation_id,
                            "needs to be a two-dimensional array!")


        mutually_exclusive = transformation.get("mutually_exclusive", None)
        #Make sure
        if isinstance(mutually_exclusive, list) and all(isinstance(item, str) for item in mutually_exclusive):
            mutually_exclusive = [transformationsIndexDict.get(trnsfrm_name) for trnsfrm_name in mutually_exclusive]
        elif mutually_exclusive is not None:
            raise Exception("The mutually_exclusive entry for", transformation_id,
                            "needs to be an array!")

        modifiers : Union[dict[str, str], None] = transformation.get("modifiers", None)
        equipment_modifiers : Union[dict[str, dict[str, str]], None] = transformation.get("equipment_modifiers", None)
        research_bonuses : Union[dict[str, str], None] = transformation.get("research_bonuses", None)
        targeted_modifiers : Union[dict[str, dict[str, str]] , None]= transformation.get("targeted_modifiers", None)
        rules : Union[dict[str, str], None] = transformation.get("rules", None)

        transformationsArray.append(transformationClass(
            index, transformation_id, localised_name, localised_desc, prerequisite, mutually_exclusive,
            modifiers, equipment_modifiers, research_bonuses, targeted_modifiers, rules
        ))
    
    return transformationsArray

#Validate data in the transformations array
def validateTransformationsArray(transformationsArray : list[transformationClass]):
    for transformation in transformationsArray:
        this_id : int = transformation.transformation_index
        
        #Validate mutually exclusives
        for mutual_transformaition in transformation.mutually_exclusive:
            if not this_id in transformationsArray[mutual_transformaition].mutually_exclusive:
                raise Exception("Transformation", transformationsArray[this_id].transformation_id,
                                "contains a mutually_exclusive argument for transformation", transformationsArray[mutual_transformaition].transformation_id,
                                "which does not contain a mutully_exclusive argument back!")
            
        #Ensure all strings are standardised
        if transformation.modifiers:
            transformation.modifiers = {
                modifier.lower() : value for modifier, value in  transformation.modifiers.items()
            }
        if transformation.equipment_modifiers:
            transformation.equipment_modifiers = {
                equipment_type.lower(): {equipment_modifier.lower(): equipment_value for equipment_modifier, equipment_value in equipment_modifiers_dict.items()}
                for equipment_type, equipment_modifiers_dict in transformation.equipment_modifiers.items()
            }
        if transformation.research_bonuses:
            transformation.research_bonuses = {
                category.lower() : value for category, value in  transformation.research_bonuses.items()
            }
        if transformation.targeted_modifiers:
            for tag in transformation.targeted_modifiers.keys():
                if len(tag) != 3:
                    raise Exception(tag, "in", transformation.transformation_id, "is not a valid tag!")

            transformation.targeted_modifiers = {
                tag.upper(): {modifier.lower(): value for modifier, value in target_dict.items()}
                for tag, target_dict in transformation.targeted_modifiers.items()
            }
        if transformation.rules:
            transformation.rules = {
                rule.lower() : value.lower() for rule, value in  transformation.rules.items()
            }

#Treat transformations like nodes in a graph - find all possible routes
def generateAllPossibleRoutes(transformationsArray : list[transformationClass]) -> list:
    nodesData = [(
        transformation.transformation_index, 
        transformation.prerequisite, 
        transformation.mutually_exclusive
        ) for transformation in transformationsArray]

    id_to_prereq : dict[list] = {nid: [set(g) for g in prereqs if g] for nid, prereqs, _ in nodesData}
    id_to_excl : dict[dict]  = {nid: set(excl) for nid, _, excl in nodesData}
    node_ids : list[int] = [nid for nid, _, _ in nodesData]

    def backtrack(route_list, route_set):
        yield route_list[:]

        for nid in node_ids:
            if nid in route_set:
                continue
            if route_set & id_to_excl[nid]: #Exclusive with something already in the route
                continue
            if any(len(g & route_set) == 0 for g in id_to_prereq[nid]): #Prerequisite logic has't been met
                continue

            route_list.append(nid)
            route_set.add(nid)
            yield from backtrack(route_list, route_set)
            route_list.pop()
            route_set.remove(nid)

    #Return generator object as list without the first (empty) entry
    #return [r for r in backtrack([], set()) if r]

    return [r for r in backtrack([], set())]

#Return an array with a single entry for our intitial idea
def getBaseIdeasArray(jsonData) -> list[ideaClass]:
    ideasArray : list[ideaClass] = []

    idea_id : str = jsonData.get("id", None)
    localised_name : str = jsonData.get("localised_name", "")
    localised_desc : str = jsonData.get("localised_desc", "")
    gfx : str = jsonData.get("gfx", "")

    use_index_variable_local : bool = jsonData.get("use_index_variable", False)
    if use_index_variable_local:
        global USE_INDEX_VARIABLE
        USE_INDEX_VARIABLE = True

    if idea_id is None or idea_id == "":
        raise Exception("Idea ID cannot be unset!")
    
    base_modifiers = jsonData.get("base_modifiers")

    modifiers : Union[dict[str, str], None] = base_modifiers.get("modifiers", None)
    equipment_modifiers : Union[dict[str, dict[str, str]], None] = base_modifiers.get("equipment_modifiers", None)
    research_bonuses : Union[dict[str, str], None] = base_modifiers.get("research_bonuses", None)
    targeted_modifiers : Union[dict[str, dict[str, str]], None] = base_modifiers.get("targeted_modifiers", None)
    rules : Union[dict[str, str], None] = base_modifiers.get("rules", None)

    ideasArray.append(ideaClass(
        idea_id, localised_name, localised_desc, gfx, modifiers, equipment_modifiers, research_bonuses, targeted_modifiers, rules
    ))

    return ideasArray

def getAllIdeasAndRoutes(ideasArray : list[ideaClass], transformationsArray : list[transformationClass], allPossibleRoutesArray : list[list[int]]):
    for index, route in enumerate(allPossibleRoutesArray):
        #print(index, ": ", route)

        if index == 0:
            continue

        #Check if the previous idea's route is equal to the current route minus the final transformation
        #If yes copy the previous idea and apply the transformation
        #Else start from the base idea and apply all transformations in the route
        if ideasArray[-1].idea_routes[0] == route[:-1]:
            ideasArray.append(copy.deepcopy(ideasArray[-1]))
            ideasArray[-1].applyTransformation(transformationsArray[route[-1]])
        else:
            ideasArray.append(copy.deepcopy(ideasArray[0]))
            for transformation in route:
                ideasArray[-1].applyTransformation(transformationsArray[transformation])

        ideasArray[-1].idea_routes[0] = route

    #ideasArray now contains a list of ideas equal in size to the number of possible routes
    #Routes can end up in the same place -> if the sets of any two ideas routes are the same, merge them
    #Store their routes so we can reverse oure way back / see how we got to the idea previously
    i : int = 1
    currentListLen : int = len(ideasArray)
    while i < currentListLen:
        j : int = i + 1
        while j < currentListLen:
            if boolAreTwoIdeasEqual(ideasArray[i], ideasArray[j]):
                ideasArray[i].idea_routes.append(ideasArray[j].idea_routes[0])
                ideasArray.pop(j)
                currentListLen -= 1   

            j += 1
        i += 1

def getAllIdeaSwaps(transformation_index : int, routeTupleToIdeaIndexDictionary : dict[tuple, int]) -> list[tuple]:
    all_idea_swaps : list[tuple] = []
    for route, idea_id in routeTupleToIdeaIndexDictionary.items():
        if len(route) > 0 and route[-1] == transformation_index:
            prev_idea_id : int = routeTupleToIdeaIndexDictionary.get(route[:-1])
            if not tuple((prev_idea_id, idea_id)) in all_idea_swaps:
                all_idea_swaps.append((prev_idea_id, idea_id))

    return list(all_idea_swaps)

def generateIfBinaryTree(arr : list[int], dic : dict[int, int], idea : str, variable_name : str, indent=1) -> str:
    indent_str : str = "\t" * indent
    
    if len(arr) == 1:
        return indent_str + f"swap_ideas = {{ remove_idea = {idea}_{arr[0]} add_idea = {idea}_{dic.get(arr[0])} }}\n"
    
    mid = len(arr) // 2
    split = (arr[mid - 1] + arr[mid]) / 2
    
    code = indent_str + "if = {\n" + indent_str + "\t" + f"limit = {{ check_variable = {{ var = {variable_name} value = {split} compare = greater_than_or_equals }} }}\n"
    code += generateIfBinaryTree(arr[mid:], dic, idea, variable_name, indent + 1)
    code += indent_str + "}\n" + indent_str + "else = {\n"
    code += generateIfBinaryTree(arr[:mid], dic, idea, variable_name, indent + 1)
    code += indent_str + "}\n"
    return code

def printScriptedEffectsFile(ideasArray : list[ideaClass], transformationsArray : list[transformationClass], routeTupleToIdeaIndexDictionary: dict[tuple, int]):
    with open(EFFECTS_FILE, "w") as outfile:
        idea_name : str = ideasArray[0].idea_id

        for transformation in transformationsArray:
            transformation_id : str = transformation.transformation_id
            transformation_index : int = transformation.transformation_index

            all_idea_swaps : list[tuple] = getAllIdeaSwaps(transformation_index, routeTupleToIdeaIndexDictionary)

            outfile.write(f"EFFECT_{idea_name}_modify_{transformation_id} = {{\n")

            #If we have an index variable, create a binary tree, otherwise just do an if-if_else block
            #Index variable method has guarenteed complexity of log(N), while without is average of .5N

            if USE_INDEX_VARIABLE:
                swap_init_values : list[int] = [swap[0] for swap in all_idea_swaps]
                swap_dictionary : dict[int, int] = {swap[0]: swap[1] for swap in all_idea_swaps}

                tree : str = generateIfBinaryTree(swap_init_values, swap_dictionary, idea_name, f"{idea_name}_idea_index")

                outfile.write(tree)

            else:
                if_prefix : str = ""
                for swap in all_idea_swaps:
                    outfile.write(f"\t{if_prefix}if = {{\n\t\tlimit = {{ has_idea = {idea_name}_{swap[0]} }}" \
                    f"\n\t\tswap_ideas = {{\n\t\t\tremove_idea = {idea_name}_{swap[0]}" \
                    f"\n\t\t\tadd_idea = {idea_name}_{swap[1]}\n\t\t}}\n\t}}\n")

                    if_prefix = "else_"
        
                outfile.write(f"\telse = {{\n\t\teffect_tooltip = {{\n\t\t\tswap_ideas = {{\n\t\t\t\tremove_idea = {idea_name}_{all_idea_swaps[0][0]}" \
                            f"\n\t\t\t\tadd_idea = {idea_name}_{all_idea_swaps[0][1]}\n\t\t\t}}\n\t\t}}\n\t}}\n")

            outfile.write(f"}}\n")
        
        outfile.write(f"EFFECT_remove_{idea_name} = {{\n")
        
        if not USE_INDEX_VARIABLE:
            if_prefix : str = ""
            for index, idea in enumerate(ideasArray):
                outfile.write(f"\t{if_prefix}if = {{ limit = {{ has_idea = {idea.idea_id}_{index} }} remove_ideas = {idea.idea_id}_{index} }}\n")
                if_prefix = "else_"

        else:
            outfile.write(f"\tmeta_effect = {{\n\t\ttext = {{\n\t\t\tremove_ideas = {idea_name}_[IDEA_INDEX]\n\t\t}}" \
                          f"\n\n\t\tIDEA_INDEX = \"[?{idea_name}_idea_index|0]\"\n\t}}\n")

            outfile.write(f"\n\tclear_variable = {idea_name}_idea_index\n")

        outfile.write(f"}}\n")


def printIdeasFile(ideasArray : list[ideaClass]):
    with open(IDEAS_FILE, "w") as outfile:
        outfile.write(f"ideas = {{\n\tcountry = {{\n")

        for idea_index, idea in enumerate(ideasArray):
            outfile.write(f"\t\t{idea.idea_id}_{idea_index} = {{\n\t\t\tremoval_cost = -1\n\n\t\t\t" \
                          f"picture = {idea.gfx}\n")
            
            if idea.modifiers:
                outfile.write(f"\n\t\t\tmodifier = {{\n")
                for key, value in idea.modifiers.items():
                    outfile.write(f"\t\t\t\t{key} = {value}\n")
                outfile.write(f"\t\t\t}}\n")

            if idea.equipment_modifiers:
                outfile.write(f"\n\t\t\tequipment_bonus = {{\n")
                for equipment_type, equipment_modifiers_dictionary in idea.equipment_modifiers.items():
                    if equipment_modifiers_dictionary:
                        outfile.write(f"\t\t\t\t{equipment_type} = {{\n")

                        for key, value in equipment_modifiers_dictionary.items():
                            outfile.write(f"\t\t\t\t\t{key} = {value}\n")
                        
                        outfile.write(f"\t\t\t\t}}\n")
                outfile.write(f"\t\t\t}}\n")

            if idea.research_bonuses:
                outfile.write(f"\n\t\t\tresearch_bonus = {{\n")
                for key, value in idea.research_bonuses.items():
                    outfile.write(f"\t\t\t\t{key} = {value}\n")
                outfile.write(f"\t\t\t}}\n")
            
            if idea.targeted_modifiers:
                for target_tag, targeted_modifiers_dictionary in idea.targeted_modifiers.items():
                    if targeted_modifiers_dictionary:
                        outfile.write(f"\n\t\t\ttargeted_modifier = {{\n\t\t\t\ttag = {target_tag}\n")

                        for key, value in targeted_modifiers_dictionary.items():
                            outfile.write(f"\t\t\t\t{key} = {value}\n")
                        
                        outfile.write(f"\t\t\t}}\n")
            
            if idea.rules:
                outfile.write(f"\n\t\t\trule = {{\n")
                for key, value in idea.rules.items():
                    outfile.write(f"\t\t\t\t{key} = {value}\n")
                outfile.write(f"\t\t\t}}\n")

            if USE_INDEX_VARIABLE:
                outfile.write(f"\n\t\t\ton_add = {{\n\t\t\t\tset_variable = {{ {idea.idea_id}_idea_index = {idea_index} }}\n\t\t\t}}\n")

            outfile.write(f"\t\t}}\n")

        outfile.write(f"\t}}\n}}")

def printLocalisationFile(ideasArray : list[ideaClass]):
    with open(LOCALISATION_FILE, "w") as outfile:
        used_names_dict : dict[str, str] = {}
        used_descs_dict : dict[str, str] = {}

        outfile.write(f"l_english:\n")

        for index, idea in enumerate(ideasArray):
            if used_names_dict.get(idea.localised_name, None) == None:
                used_names_dict[idea.localised_name] = f"{idea.idea_id}_{index}"
                outfile.write(f"\n {idea.idea_id}_{index}: \"{idea.localised_name}\"")
            else:
                outfile.write(f"\n {idea.idea_id}_{index}: \"${used_names_dict.get(idea.localised_name)}$\"")
            
            if used_descs_dict.get(idea.localised_desc, None) == None:
                used_descs_dict[idea.localised_desc] = f"{idea.idea_id}_{index}"
                outfile.write(f"\n {idea.idea_id}_{index}_desc: \"{idea.localised_desc}\"")
            else:
                outfile.write(f"\n {idea.idea_id}_{index}_desc: \"${used_descs_dict.get(idea.localised_desc)}_desc$\"")

def main():
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open("idea.json", "r", encoding="utf-8") as json_file:
        jsonData = json.load(json_file)

    ideasArray : list[ideaClass] = getBaseIdeasArray(jsonData)

    #We now have our base idea - get the transformations
    transformationsJson = jsonData.get("transformations", None)
    if transformationsJson is None:
        raise Exception("The idea", ideasArray.idea_id, "has no transformations!")
    
    transformationsIndexDict : dict[str, int] = getTransformationsIndexDictionary(transformationsJson)
    transformationsArray : list[transformationClass] = getTransformationsArray(transformationsJson, transformationsIndexDict)

    #We now have our transformations array - validate the data
    validateTransformationsArray(transformationsArray)

    #Now begin creating the routes
    allPossibleRoutesArray : list[list[int]] = generateAllPossibleRoutes(transformationsArray)

    getAllIdeasAndRoutes(ideasArray, transformationsArray, allPossibleRoutesArray)

    #Get a dictionary that takes a unique tuple for the route and points to the idea it gives
    routeTupleToIdeaIndexDictionary: dict[tuple, int] = {
        tuple(route): index
        for index, idea in enumerate(ideasArray)
        for route in idea.idea_routes
    }
    
    printScriptedEffectsFile(ideasArray, transformationsArray, routeTupleToIdeaIndexDictionary)
    printIdeasFile(ideasArray)
    printLocalisationFile(ideasArray)
    

if __name__ == "__main__":
    main()
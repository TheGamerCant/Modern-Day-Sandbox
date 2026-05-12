import random
import os
import pyperclip
from pathlib import Path
from time import perf_counter

def main():
    time_start: float = perf_counter()
    
    filled_colours: set[tuple[int, int, int]] = set()
    
    definition_file: Path = Path.cwd().parents[1] / "map/definition.csv"
    with open(str(definition_file)) as f:
        lines: list[str] = f.readlines()

        for i, line in enumerate(lines):
            prov_data: list[str] = line.split(";")
            if len(prov_data) > 3:
                filled_colours.add((prov_data[1], prov_data[2], prov_data[3]))


    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s\n\nCommands:\nn - Print new colour\nc1 - Copy last colour RGB to clipboard\nc2 - Copy last colour Hex to clipboard\nq - Quit\n")

    r: int = random.randrange(0, 256)
    g: int = random.randrange(0, 256)
    b: int = random.randrange(0, 256)
    h: str = format(((r << 16) + (g << 8) + b), '#08x')
            
    while True:
        user_input: str = input("").lower()

        if user_input == "q":
            break

        elif user_input == "n":
            while (r, g, b) in filled_colours:
                r = random.randrange(0, 256)
                g = random.randrange(0, 256)
                b = random.randrange(0, 256)

            filled_colours.add((r, g, b))

            h = format(((r << 16) + (g << 8) + b), '#08x')
            
            print(f"{str(r)};{str(g)};{str(b)} - {h[2:]}")
            
        elif user_input == "c1":
            pyperclip.copy(f"{str(r)};{str(g)};{str(b)}")
            
        elif user_input == "c2":
            pyperclip.copy(h[2:])
        
    

if __name__ == "__main__":
    main()

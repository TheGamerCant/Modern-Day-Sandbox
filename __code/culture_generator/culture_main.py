import json
from pathlib import Path

class Culture:
    def __init__(
            self,
            token: str,
            name: str = "",
            desc: str = "",
            red: int = 0,
            green: int = 0,
            blue: int = 0,
            super_culture: str | None = None
        ):
        self.token: str = token
        self.name: str = name
        self.desc: str = desc
        self.red: int = red
        self.green: int = green
        self.blue: int = blue
        self.super_culture: str | None = super_culture

    def __str__(self):
        return (
            f"Token: {self.token}\nName: {self.name}\nRed: {self.red}\n"
            f"Green: {self.green}\nBlue: {self.blue}\nSuper Culture: {self.super_culture}\n"
        )

def WriteOnActionsFile

def LoadCulturesFromJson() -> list[Culture]:
    culture_list: list[Culture] = []

    with open("cultures.json", "r", encoding="utf-8") as json_file:
        json_data = json.load(json_file)

        for culture in json_data.get("cultures"):
            culture_list.append(Culture(
                token = culture.get("token"),
                red = culture.get("red", 0),
                green = culture.get("green", 0),
                blue = culture.get("blue", 0),
                name = culture.get("name", ""),
                desc = culture.get("desc", ""),
                super_culture = culture.get("super_culture", None)
            ))

    return culture_list

def main():
    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    on_actions_file: Path = mod_directory / "common/on_actions/TDA_culture_on_actions.txt"
    idea_tokens_file: Path = mod_directory / "common/ideas/TDA_culture_tokens.txt"
    localisation_file: Path = mod_directory / "localisation/english/TDA_culture_l_english.yml"
    states_folder: Path = mod_directory / "history/states"

    cultures_list: list[Culture] = LoadCulturesFromJson()

    for culture in cultures_list:
        print(culture)


if __name__ == "__main__":
    main()
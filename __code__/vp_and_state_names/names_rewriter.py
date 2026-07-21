import re
from typing import Any
import json

def load_file_to_string(filename: str):
    try:
        with open(filename, "r", encoding="utf-8") as file:
            variables: dict[str, str] = {}
            result: list[str] = []

            for line in file:
                line = line.rstrip("\n")

                in_single_quotes = False
                in_double_quotes = False

                processed_line = []

                # Remove comments outside quotes
                i = 0
                while i < len(line):
                    c = line[i]

                    if (
                        c == "'"
                        and not in_double_quotes
                        and (i == 0 or line[i - 1] != "\\")
                    ):
                        in_single_quotes = not in_single_quotes

                    elif (
                        c == '"'
                        and not in_single_quotes
                        and (i == 0 or line[i - 1] != "\\")
                    ):
                        in_double_quotes = not in_double_quotes

                    if c == "#" and not in_single_quotes and not in_double_quotes:
                        break

                    processed_line.append(c)
                    i += 1

                processed_line = "".join(processed_line).strip()

                # Handle variable definition
                if processed_line.startswith("@"):
                    eq = processed_line.find("=")

                    if eq != -1:
                        var_name = processed_line[1:eq].strip()
                        var_value = processed_line[eq + 1:].strip()

                        variables[var_name] = var_value

                    continue

                # Replace variable references
                for name, value in variables.items():
                    token = "@" + name
                    processed_line = processed_line.replace(token, value)

                result.append(processed_line)

            return "\n".join(result)

    except OSError:
        raise RuntimeError(f"Failed to open file: {filename}")


def parse_clausewitz(text: str) -> dict[Any, Any]:
    tokens = re.findall(r'"[^"]*"|[{}=]|[^\s{}=]+', text)

    def add_value(obj, key, value):
        """Preserve duplicate keys as lists."""
        if key in obj:
            if not isinstance(obj[key], list):
                obj[key] = [obj[key]]
            obj[key].append(value)
        else:
            obj[key] = value

    def parse_value(i):
        token = tokens[i]

        # Parse block
        if token == "{":
            i += 1
            obj = {}
            arr = []

            has_keys = False

            while tokens[i] != "}":
                # key = value
                if i + 1 < len(tokens) and tokens[i + 1] == "=":
                    has_keys = True

                    key = tokens[i]
                    i += 2

                    value, i = parse_value(i)
                    add_value(obj, key, value)

                else:
                    value, i = parse_value(i)
                    arr.append(value)

            i += 1  # skip }

            return (obj if has_keys else arr), i

        # Quoted string
        elif token.startswith('"') and token.endswith('"'):
            return token[1:-1], i + 1

        # Integer
        elif re.fullmatch(r"-?\d+", token):
            return int(token), i + 1

        # Float
        elif re.fullmatch(r"-?\d+\.\d+", token):
            return float(token), i + 1

        # Bare identifier
        else:
            return token, i + 1

    result = {}
    i = 0

    while i < len(tokens):
        if i + 1 < len(tokens) and tokens[i + 1] == "=":
            key = tokens[i]
            i += 2

            value, i = parse_value(i)
            add_value(result, key, value)
        else:
            i += 1

    return result


def main():
    victory_point_names = parse_clausewitz(load_file_to_string("/Users/charles/Documents/GitHub/Modern-Day-Sandbox/__code__/Map_Editor/in/names_victory_points.txt"))
    state_names = parse_clausewitz(load_file_to_string("/Users/charles/Documents/GitHub/Modern-Day-Sandbox/__code__/Map_Editor/in/names_states.txt"))

    out_json = {
        "state_names": [],
        "victory_point_names": [],
    }

    for state_id, data in state_names.items():
        entry_dict = {
            "id": int(state_id),
            "default_name": data.get("default", f"STATE_{state_id}"),
            "comment": "",
            "custom_names": []
        }

        if custom_names := data.get("entry"):
            if isinstance(custom_names, dict):
                custom_names = [custom_names]

            for custom_name in custom_names:
                entry_dict["custom_names"].append({
                    "requirements": " ".join(custom_name.get("requirements")),
                    "name": custom_name.get("name")
                })

        out_json["state_names"].append(entry_dict)

    for vp_id, data in victory_point_names.items():
        if isinstance(data, list):
            print(vp_id, data)
        entry_dict = {
            "id": int(vp_id),
            "default_name": data.get("default", f"VICTORY_POINTS_{vp_id}"),
            "comment": "",
            "custom_names": []
        }

        if custom_names := data.get("entry"):
            if isinstance(custom_names, dict):
                custom_names = [custom_names]

            for custom_name in custom_names:
                entry_dict["custom_names"].append({
                    "requirements": " ".join(custom_name.get("requirements")),
                    "name": custom_name.get("name")
                })

        out_json["victory_point_names"].append(entry_dict)

    with open("names.json", "w", encoding="utf-8") as json_file:
        json_file.write(json.dumps(out_json, indent="\t", ensure_ascii=False))


if __name__ == "__main__":
    main()
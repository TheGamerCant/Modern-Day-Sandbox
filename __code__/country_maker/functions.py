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


def get_map_from_brackets(obj_in: str | list[str] | None):
    def parse_blocks(text: str) -> dict[str, list[str]]:
        result: dict[str, list[str]] = {}

        i: int = 0
        n: int = len(text)

        while i < n:
            # Skip whitespace
            while i < n and text[i].isspace():
                i += 1

            # Read key
            key_start = i
            while i < n and text[i] not in "= \t\n":
                i += 1

            key = text[key_start:i].strip()

            # Skip until opening brace
            while i < n and text[i] != '{':
                i += 1

            if i >= n:
                break

            # Parse matching braces
            depth = 0
            value_start = i + 1

            while i < n:
                if text[i] == '{':
                    depth += 1
                elif text[i] == '}':
                    depth -= 1

                    if depth == 0:
                        value = text[value_start:i].strip()

                        if key in result:
                            result[key].append(value)
                        else:
                            result[key] = [value]

                        i += 1
                        break

                i += 1

        return result

    results: dict[str, list[str]] = {}

    if isinstance(obj_in, str):
        results = parse_blocks(obj_in)
    elif isinstance(obj_in, list):
        for entry in obj_in:
            tmp_results = parse_blocks(entry)

            for key, value in tmp_results.items():
                if key in results:
                    results[key].extend(value)
                else:
                    results[key] = value

    return results

def clamp(n, min_val, max_val):
    if n < min_val:
        return min_val
    elif n > max_val:
        return max_val
    else:
        return n
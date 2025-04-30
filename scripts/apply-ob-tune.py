"""
    This script updates the tunablevalues.def file with SPSA outputs from OpenBench.
    Paste the outputs retrieved from the tune to this script and then press enter.
    The script will automatically apply the changes to the tunables file.
"""

import os
import re

SCRIPTS_DIR = os.path.dirname(os.path.realpath(__file__))
SOURCE_DIR = os.path.realpath(f"{SCRIPTS_DIR}/..")
TUNABLES_PATH = os.path.realpath(f"{SOURCE_DIR}/illumina/tunablevalues.def")


def main():
    changed_values = {}
    while True:
        s = input()

        if len(s) == 0:
            break

        [name, value] = s.split(",")
        changed_values[name.replace("TUNABLE_", "").replace("_FP", "")] = value.strip()

    lines = []
    with open(TUNABLES_PATH, "r") as f:
        for line in f:
            lines.append(line)

    for i in range(len(lines)):
        line = line_prev = lines[i].strip()
        if not line.startswith("TUNABLE_VALUE("):
            continue

        args_str = re.match(r".*\((.*\,.*\,.*)\)", line).groups()[0]
        args = args_str.split(', ')
        name = args[0]
        if name not in changed_values.keys():
            continue

        args[2] = changed_values[name]

        lines[i] = lines[i].replace(args_str, ", ".join(args))

    with open(TUNABLES_PATH, "w") as f:
        f.write("".join(lines))


if __name__ == "__main__":
    main()

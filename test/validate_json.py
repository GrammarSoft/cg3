import json
from pathlib import Path
import re
import sys
import subprocess
import difflib # Added import

try:
    import jsonschema
except ImportError:
    print("Warning: jsonschema library not found. Skipping JSON validation.")
    print("Install it with: pip install jsonschema")
    sys.exit(0)

# --- Configuration ---
SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
SCHEMA_PATH = SCRIPT_DIR / "cg3-cohort.schema.json"
CG_CONV_PATH = (ROOT_DIR / "src" / "cg-conv").resolve()
CG_CONV_EXTRA_ARGS = ["--parse-dep", "--deleted"]
assert CG_CONV_PATH.is_file(), f"cg-conv not found at {CG_CONV_PATH}. Make sure it is built."
INPUT_GLOB_PATTERN = "**/input.txt"

# --- End Configuration ---

# Add a timeout value (in seconds)
PROCESS_TIMEOUT = 5 # Adjust as needed


def strip_deps(string):
    """Strips dependencies from a string."""
    return re.sub(r'\s*#\d+->\d+', '', string)


def jsonl_has_validation_errors(output_lines, validator, filename):
    """Validates each line of JSONL output."""
    has_errors = False
    for i, line in enumerate(output_lines):
        line = line.strip()
        if not line:
            continue
        try:
            instance = json.loads(line)
            validator.validate(instance)
        except json.JSONDecodeError as e:
            print(f"ERROR: Failed to decode JSON on line {i+1} of output for {filename}: {e}")
            print(f"       Line content: {line}")
            has_errors = True
        except jsonschema.ValidationError as e:
            print(f"ERROR: Schema validation failed on line {i+1} of output for {filename}:")
            print(f"       Error: {e.message}")
            print(f"       Path: {list(e.path)}")
            print(f"       Instance: {e.instance}")
            has_errors = True
        except Exception as e:
            print(f"ERROR: Unexpected error validating line {i+1} for {filename}: {e}")
            has_errors = True
    return has_errors


print(f"Loading schema: {SCHEMA_PATH}")
with open(SCHEMA_PATH, 'r') as f:
    schema = json.load(f)
validator = jsonschema.Draft7Validator(schema)

print(f"Searching for input files matching '{INPUT_GLOB_PATTERN}' in {SCRIPT_DIR}")
input_files = list(SCRIPT_DIR.rglob(INPUT_GLOB_PATTERN))

if not input_files:
    print("Warning: No input.txt files found.")
    sys.exit(0)

print(f"Found {len(input_files)} input files.")
overall_to_fro_errors = 0
overall_json_validation_errors = 0
output = {}

skipped_dirs = ["Apertium/"]  # TODO add T_Variables/ and T_InputCommands/
skipped_dirs_regex = "|".join([re.escape(dir) for dir in skipped_dirs])
print(f"Skipping directories: {skipped_dirs}")
for input_file in input_files:
    if re.search(skipped_dirs_regex, str(input_file)):
        continue

    with open(input_file, 'r', encoding='utf-8') as f:
        input_content = f.read()

    cC_process = subprocess.run(
        [str(CG_CONV_PATH), "-c", "-C"] + CG_CONV_EXTRA_ARGS,
        input=input_content, # Pass file content as stdin
        capture_output=True,
        text=True,
    )
    output["cC"] = {"in": input_content,
                    "process": cC_process,
                    "out_str": strip_deps(cC_process.stdout)}

    for stream_format in "j":  # TODO a, f and n
        to_label = f'c{stream_format.upper()}'
        to_process = subprocess.run(
            [str(CG_CONV_PATH), "-c", f"-{stream_format.upper()}"] + CG_CONV_EXTRA_ARGS,
            input=cC_process.stdout,
            capture_output=True,
            text=True,
        )
        output[to_label] = {"process": to_process,
                            "out_str": strip_deps(to_process.stdout)}
        fro_label = f'{stream_format}C'
        fro_process = subprocess.run(
            [str(CG_CONV_PATH), f"-{stream_format}", "-C"] + CG_CONV_EXTRA_ARGS,
            input=to_process.stdout,
            capture_output=True,
            text=True,
        )
        output[fro_label] = {"process": fro_process,
                             "out_str": strip_deps(fro_process.stdout)}
        if output["cC"]["out_str"] != output[fro_label]["out_str"]:
            print(f"ERROR: cg-conv cC output differs from {to_label}->{fro_label} output for {input_file}")
            # Generate and print a unified diff
            diff = difflib.unified_diff(
                output["cC"]["out_str"].splitlines(keepends=True),
                output[fro_label]["out_str"].splitlines(keepends=True),
                fromfile='cC_output',
                tofile=f'{fro_label}_output',
                lineterm='\n'
            )
            print("       Differences:")
            for line in diff:
                print(f"       {line}", end="") # Write directly to preserve formatting
            intermediate_stream = output[to_label]["process"].stdout.strip()
            print(f"       Intermediate stream:{intermediate_stream}\n\n{"="*79}")
            overall_to_fro_errors += 1
            continue

    output_lines = output["cJ"]["process"].stdout.strip().split('\n')
    if jsonl_has_validation_errors(output_lines, validator, str(input_file)):
        overall_json_validation_errors += 1


if overall_to_fro_errors and overall_json_validation_errors:
    print(f"\nThere were...\n"
          f"    {overall_to_fro_errors} round trip conversion errors.\n"
          f"    {overall_json_validation_errors} JSON validation errors.")
    sys.exit(3)
elif overall_to_fro_errors:
    print(f"\nThere were {overall_to_fro_errors} round trip conversion errors.")
    sys.exit(1)
elif overall_json_validation_errors:
    print(f"\nThere were {overall_json_validation_errors} JSON validation errors.")
    sys.exit(2)
else:
    print("Round trip conversion and JSON validation finished successfully.")
    sys.exit(0)

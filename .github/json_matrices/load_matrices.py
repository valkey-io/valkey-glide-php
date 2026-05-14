#!/usr/bin/env python3
import json
import os
import sys
from pathlib import Path

def load_json(filename: str) -> list | dict:
    return json.loads(Path(filename).read_text())

PROFILES = load_json("profiles.json")

def filter_by_profile(entries: list, profile: str) -> list:
    return [e for e in entries if profile in e.get("profiles", [])]

def main() -> None:
    if len(sys.argv) != 2 or sys.argv[1] not in PROFILES:
        print(f"Usage: {sys.argv[0]} <{'|'.join(PROFILES)}>", file=sys.stderr)
        sys.exit(1)

    profile = sys.argv[1]

    build_matrix = filter_by_profile(load_json("build-matrix.json"), profile)
    host = [h for h in build_matrix if "IMAGE" not in h]
    container_host = [h for h in build_matrix if "IMAGE" in h]

    engine = filter_by_profile(load_json("engine-matrix.json"), profile)
    assert engine, "Given profile resulted in empty engine matrix."

    lang_versions = load_json("supported-languages-versions.json")
    php_entry = next(e for e in lang_versions if e["language"] == "php")
    php = [e["version"] for e in filter_by_profile(php_entry["versioned"], profile)]
    assert php, "Given profile resulted in empty php version matrix."

    configs = {
        "host-matrix": json.dumps(host),
        "container-host-matrix": json.dumps(container_host),
        "engine-matrix": json.dumps(engine),
        "php-matrix": json.dumps(php),
    }

    print(f"Loaded matrices for profile '{profile}':")
    github_output = os.environ.get("GITHUB_OUTPUT")
    if github_output:
        with open(github_output, "a") as f:
            for key, value in configs.items():
                print(f"{key}={value}", file=f)
    else:
        for key, value in configs.items():
            print(f"{key}={value}")

if __name__ == "__main__":
    main()

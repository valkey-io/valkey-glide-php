import os
import re

def remove_optional_from_proto(directory):
    proto_file_pattern = re.compile(r'.*\.proto$')

    for root, _, files in os.walk(directory):
        for file in files:
            if proto_file_pattern.match(file):
                filepath = os.path.join(root, file)

                with open(filepath, 'r') as f:
                    content = f.read()

                new_content = re.sub(r'\boptional\s+', '', content)

                with open(filepath, 'w') as f:
                    f.write(new_content)

                print(f"Updated {filepath}")

# Usage
if __name__ == "__main__":
    directory_to_scan = "../valkey-glide/glide-core/src/protobuf/"
    remove_optional_from_proto(directory_to_scan)

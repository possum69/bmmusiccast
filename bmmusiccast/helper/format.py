import json
import os

files = os.listdir(".")

for file in files:
    if file.endswith(".json"):
        try:
            with open(file, 'r') as f:
                data = json.load(f)

            with open(file, 'w') as f:
                print(json.dumps(data, indent=4), file=f)
        except Exception as e:
            print(f"Error processing {file}: {e}")
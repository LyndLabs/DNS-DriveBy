import re

# Define the regex pattern to match base32 strings that are at least 20 bytes long
pattern = re.compile(b'[A-Z2-7]{20,}')

# Open the binary file in binary mode
with open('dump.bin', 'rb') as f:
    # Read the entire file into a bytes object
    data = f.read()

    # Search for matches using the regex pattern
    matches = pattern.findall(data)

    # Print the matches
    for match in matches:
        print(match)

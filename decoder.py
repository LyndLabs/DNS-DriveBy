import base64

# Read the base32 string from user input
base32_string = input("Enter a base32 string to decode: ")

# Define the chunk size
chunk_size = 61

# Substring the base32 string into chunks of 61 bytes
chunks = [base32_string[i:i+chunk_size] for i in range(0, len(base32_string), chunk_size)]

# Iterate over each chunk and decode it to plain text
for chunk in chunks:
    # Ignore any padding issues by adding missing padding
    while len(chunk) % 8 != 0:
        chunk += "="
    
    # Decode the chunk into a bytes object
    data = base64.b32decode(chunk)
    
    # Decode the bytes object using UTF-8 encoding
    decoded = data.decode('utf-8')
    
    # Print the decoded information
    print(decoded)

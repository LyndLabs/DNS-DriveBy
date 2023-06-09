import csv
import re

# Define the regular expression pattern to match MAC addresses
mac_pattern = r'^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$'

# Open the input and output CSV files
with open('output_file.csv', 'r') as input_file, open('output_file_2.csv', 'w', newline='') as output_file:
    # Create a CSV reader and writer objects
    csvreader = csv.reader(input_file)
    csvwriter = csv.writer(output_file)
    # Initialize the previous line as an empty list
    prev_line = []
    # Iterate over each row in the input CSV file
    for row in csvreader:
        # Check if the 3rd parameter in the row matches the MAC address pattern
        if re.match(mac_pattern, row[2]):
            # Write the first two parameters and [MEOW] to the output CSV file
            csvwriter.writerow(row[:2] + ['[MEOW]'] + prev_line[3:])
            # Append the MAC address and the remaining parameters to the last row in the output CSV file
            csvwriter.writerow([row[2]] + row[3:])
        else:
            # If the 3rd parameter is not a MAC address, write the original row to the output CSV file
            csvwriter.writerow(row)
            # Store the previous line for future use
            prev_line = row


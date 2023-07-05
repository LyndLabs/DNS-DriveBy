import csv
import glob

# create an empty list to store the rows
combined_rows = []

# loop over each CSV file
for filename in glob.glob('*.csv'):
    with open(filename, 'r') as f:
        # skip the first two lines
        next(f)
        next(f)
        # read in the remaining rows
        reader = csv.reader(f)
        rows = list(reader)
        # add the rows to the combined list
        combined_rows.extend(rows)

# write out the combined rows to a new CSV file
with open('combined.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(combined_rows)


import csv
import base64
from datetime import datetime

burst_size = 0
current_burst = 1
last_value = 0

with open('Buena_Park_WarSkate.csv', 'r') as csvfile:
    csvreader = csv.reader(csvfile)
    header = next(csvreader)  # skip the first line
    with open('burst_sizes.txt', 'w') as outfile, open('burst_data.txt', 'a') as datafile:
        for row in csvreader:
            value = int(row[4])
            if value < last_value:
                # New burst, write burst size to file
                outfile.write(f"{burst_size}\n")
                burst_size = 0
                current_burst += 1
            burst_size += 1
            last_value = value
            if burst_size == 1:
                # Write current row data to file
                lat = round(float(row[6]), 5)
                lng = round(float(row[7]), 5)
                dt = datetime.strptime(row[3], '%Y-%m-%d %H:%M:%S')
                formatted_dt = dt.strftime('%m/%d/%y-%H:%M:%S')
                data_bytes = f"{lat:.5f},{lng:.5f},{formatted_dt}".encode('utf-8')
                b32_encoded = base64.b32encode(data_bytes).decode('utf-8').rstrip("=")
                datafile.write(f"{b32_encoded}\n")
        
        # Write size of last burst to file
        outfile.write(f"{burst_size}\n")


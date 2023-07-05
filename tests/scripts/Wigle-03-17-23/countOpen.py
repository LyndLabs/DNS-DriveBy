import csv

# Open the CSV file
with open('WiGLE-20230317.csv', newline='') as csvfile:
    wifi_reader = csv.reader(csvfile, delimiter=',')
    open_networks = set()
    for row in wifi_reader:
        
        if row[1] not in open_networks:
                # Add the MAC address to the set of open networks
            open_networks.add(row[1])
            print(row[1])
    # Print the number of unique open networks detected
    print('Number of unique open networks detected:', len(open_networks))

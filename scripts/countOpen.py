import csv

# Open the CSV file
with open('Buena_Park_WarSkate.csv', newline='') as csvfile:
    wifi_reader = csv.reader(csvfile, delimiter=',')

    # Create a set to store the unique MAC addresses of open networks
    open_networks = set()

    # Iterate through each row in the CSV file
    for row in wifi_reader:
        # Check if the encryption type is AUTO or NONE
        
            # Check if the MAC address has not already been counted
        if row[0] not in open_networks:
            if 'LaserJet' not in row[1] and not row[1].startswith('DIRECT') and not row[1].startswith('HP'):
                # Add the MAC address to the set of open networks
               open_networks.add(row[0])
               print(row[1])
    # Print the number of unique open networks detected
    print('Number of unique open networks detected:', len(open_networks))

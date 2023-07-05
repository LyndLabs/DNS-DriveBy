# combine & clean CanaryTokens CSV files

import os
import pandas as pd

path = './'
files = [file for file in os.listdir(path) if file.endswith('.csv')]

df = pd.read_csv(path + files[0])

for file in files[1:]:
    df_temp = pd.read_csv(path + file)
    df = df.append(df_temp, ignore_index=True)

# remove duplicates & sort by timestamp
df = df.sort_values('Timestamp', ascending=True)
df.drop_duplicates(subset=['Timestamp'], inplace=True)

df.to_csv('CanaryTokens-031723.csv', index=False)
print("Unique Entries: ", len(df))

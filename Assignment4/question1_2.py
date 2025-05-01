import pandas as pd

# 
from utils import get_prefix

# load file into data frame table
df = pd.read_csv('./netflow.csv')

# get the prefix for the Src IP addr column
df['Src IP addr'] = df['Src IP addr'].apply(get_prefix)


# Number of flows for each ip prefix (descending)
flow_counts = df['Src IP addr'].value_counts()
top_ten_flows = flow_counts[:10]
top_ten_ips = top_ten_flows.index
print("These are the top 10 IP addresses based on flow counts:")
print(top_ten_ips.to_list())
print() # newline

top_ten_sum = top_ten_flows.sum()
percent = top_ten_sum / len(df)
print("This is the percentage:", percent * 100, '%')
print() # newline
# print(percent * 100)

# ------ #
bytes_per_ip = df.groupby('Src IP addr')['Bytes'].sum()
sort_bpi = bytes_per_ip.sort_values(ascending=False)
top_ten_byte_ips = sort_bpi[:10]
print("These are the top 10 IP addresses based on bytes:")
print(top_ten_byte_ips.index.to_list())
print() # newline

total_bytes = df['Bytes'].sum()
top_ten_sum = top_ten_byte_ips.sum()

byte_percent = top_ten_sum / total_bytes * 100
print("Byte Percentage:", byte_percent, '%')
# print(byte_percent)


# total_flows = len(df)
# print()
# print("This is their percentage of all flows")
# print()
 







# Get the number of flows for each source IP address, only considering its 16-bit prefix (for example: 
#         the IP addresses 255.255.0.1 and 255.255.0.2 are counted as there being two 255.255 addresses). 
#         What are the top ten IP address prefixes, and what percentage of all the flows recorded are they involved 
#         in? (No need to report the percentage per source IP address, just report the aggreagate percentage for top 
#         ten source IP addresses.)

# sort by the src IP address
# figure out how to calculate these groups


# Now, aggregate the number of bytes by source IP addresses the same way. What are the top ten IP address prefixes 
# in this case, and what percentage of bytes sent across all flows are they responsible for (no need to report the 
# percentage per source IP address, just report the aggreagate percentage for top ten source IP addresses)?

# figure our how to get this group information
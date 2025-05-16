# address block 128.112.0.0
# what % of bytes are sent from
#         this router?
# What % of bytes are sent to 
#         this router?
# What percentage of bytes have
#         the above same src/dst
#         IPs
# What does this tell you about
#         the router traffic

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

from utils import get_prefix

# read data frame
df = pd.read_csv('./netflow.csv')
# update prefixes
df['Src IP addr'] = df['Src IP addr'].apply(get_prefix)
df['Dst IP addr'] = df['Dst IP addr'].apply(get_prefix)

# Src and Dst by # of Bytes
bytes_per_ip_src = df.groupby('Src IP addr')['Bytes'].sum()
bytes_per_ip_dst = df.groupby('Dst IP addr')['Bytes'].sum()

total_bytes = df['Bytes'].sum()

# router bytes
router_src_bytes = bytes_per_ip_src['128.112']
router_dst_bytes = bytes_per_ip_dst['128.112']

# calc percentages
src_percent = router_src_bytes / total_bytes * 100
dst_percent = router_dst_bytes / total_bytes * 100

print("src percent:", src_percent, '%')
print("dst Precent:", dst_percent, '%')
print() # newline

src_dst_bytes = df[(df['Src IP addr'] == '128.112') & (df['Dst IP addr'] == '128.112')]
sum_bytes = src_dst_bytes['Bytes'].sum()

src_dst_percent = sum_bytes / total_bytes * 10
print("Same src/dst bytes:", src_dst_percent, '%')
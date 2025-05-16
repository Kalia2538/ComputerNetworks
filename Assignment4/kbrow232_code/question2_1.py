import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from collections import Counter

# read data frame
df = pd.read_csv('./bgp_route.csv')

full_as_list = []
for p in df['ASPATH']:
        # sepaate each AS in path
        nums = p.split() 
        # add path to the full list
        full_as_list.extend(nums)

# we should have the full as list
frequencies = Counter(full_as_list)

top_10_as = frequencies.most_common(10)
top_10_nums =  [val[0] for val in top_10_as]
print("The top 10 ASes are:")
print(top_10_nums)

total_paths = len(df)

nums_top_10 = [as_num for as_num, _ in top_10_as]

counter = 0
for p in df['ASPATH']:
        as_nums = p.split()
        if any(top_10 in nums_top_10 for top_10 in as_nums):
                counter+=1

top_10_paths = counter
total_paths = len(df)

path_percent = top_10_paths / total_paths * 100
print("This is the percentage of paths that have a top 10 AS:", path_percent, '%')


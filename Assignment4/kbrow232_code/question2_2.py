import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv('./bgp_route.csv')

# length of each unique AS w/i the path

path_lens = []
for p in df['ASPATH']:
        as_nums = p.split()
        # gets rid of duplicates
        unique_as = set(as_nums)
        path_lens.append(len(unique_as))

# we have a list with the length of every path

sort_path_lens = np.sort(path_lens)


cdf = np.arange(1, len(sort_path_lens) + 1) / len(sort_path_lens)

plt.plot(sort_path_lens, cdf)
plt.xlabel('Path Length')
plt.ylabel('Proportion of paths with this length or less')
plt.title('CDF of Unique Path Lengths')
plt.grid('True')
plt.show()

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# read data from file
df_route = pd.read_csv('./bgp_route.csv')
df_update = pd.read_csv('./bgp_update.csv')

# need to extract the AS number from the FROM column - put into AS column
df_route['AS'] = df_route['FROM'].str.extract(r'AS(\d+)').astype(int)
df_update['AS'] = df_update['FROM'].str.extract(r'AS(\d+)').astype(int)

total_updates = len(df_update)
updates_per_as = df_update['AS'].value_counts()


full_as_list = df_route.groupby('AS')

sorted_upas = updates_per_as.values
cumulative_sum_updates = np.cumsum(sorted_upas) 


x_axis = np.arange(1, len(full_as_list) + 1) / len(full_as_list) * 100
y_axis = cumulative_sum_updates / total_updates * 100


plt.plot(x_axis, y_axis)
plt.xlabel('Top Percentage of ASes')
plt.ylabel('Cumulative Percentage of Updates')
plt.title('CDF of Updates per AS')
plt.grid('True')
plt.show()
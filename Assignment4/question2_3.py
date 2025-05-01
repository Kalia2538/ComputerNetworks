import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv('./bgp_update.csv')

df[['minutes', 'seconds']] = df['TIME'].str.split(':', expand = True)

df['minutes'] = df['minutes'].astype(int)


# calcs the # of updates per minute
updates_per_minute = df.groupby('minutes').size()
avg = updates_per_minute.mean()

print("The average number of updates per minute is:", avg)


# putting the 0 indexed value at the end in index 60 (for the continuity of the graph)
num = updates_per_minute.values[0]
updates_per_minute = updates_per_minute.drop(0)
updates_per_minute.loc[60] = num


plt.plot(updates_per_minute.index, updates_per_minute.values)
plt.xlabel('Minute')
plt.ylabel('# of Updates')
plt.title('Updates per Minute')
plt.grid('True')

plt.axhline(y = avg, color = 'blue',  linestyle = '--', label = f'Average = {avg:.2f}')
plt.legend()
plt.show()

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
# 
from utils import plot_cdf

# load file into data fram table
df = pd.read_csv('./netflow.csv')

# TODO: do I need to make sure the file data is formatted well (error check?)
# sort the data based on bytes
df = df.sort_values(by = 'Bytes')

plot_cdf(df['Bytes'], 'CDF of All Flows')
# extract the UDP rows
df_bytes_udp = df[df['Protocol'] == 'UDP']
plot_cdf(df_bytes_udp['Bytes'], 'CDF of UDP Flows')
# extract the TCP rows
df_bytes_tcp = df[df['Protocol'] == 'TCP']
plot_cdf(df_bytes_tcp['Bytes'], 'CDF of TCP Flows')

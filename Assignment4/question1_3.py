import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
# 
from utils import plot_cdf

# Picking port number 53 - used for the Domain Name Service (DNS)

# load file into data frame table
df = pd.read_csv('./netflow.csv')

total_flows = len(df)
src_ports = df['Src port'].value_counts()
dst_ports = df['Dst port'].value_counts()
print("Src ports:", src_ports[53])
print("dst_ports:", dst_ports[53])
print("total flows:", total_flows)
src_percent = src_ports[53] / total_flows * 100
dst_percent = dst_ports[53] / total_flows * 100
print("src percent:", src_percent, '%')
print("dst percent:", dst_percent, '%')
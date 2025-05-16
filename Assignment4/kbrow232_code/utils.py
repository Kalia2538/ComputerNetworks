import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

# given a sorted data set and a label, plot a cdf graph
# of the data (x axis is the bytes)
def plot_cdf(data, title): # possibly add a title parameter
        # Sort the data (based on)
        cdf = np.arange(1, len(data) + 1) / len(data)

        plt.plot(data, cdf)
        # set the x axis label
        plt.xlabel('Number of Bytes in Flow')
        # set the y axis label
        # TODO: may need to clean up this wording
        plt.ylabel('Percentage of Flows with <= x Bytes')
        # set the title label
        plt.title(title)
        plt.xscale('log')
        plt.grid('True')
        plt.show()

# returns /16 from IP address
def get_prefix(full_ip):
        # for type checking, but not really necessary
        full_ip = str(full_ip)
        split = full_ip.split('.')
        prefix = split[0] + '.' + split[1]
        
        return prefix
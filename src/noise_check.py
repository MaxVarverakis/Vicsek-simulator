import numpy as np
import matplotlib.pyplot as plt

fs = 18

data = np.loadtxt("data/noise_0.txt", skiprows=0)

plt.hist(data, bins=50, edgecolor='k')
plt.xlabel("Noise value", fontsize=fs)
plt.ylabel("Count", fontsize=fs)
plt.show()

# plt.scatter([i for i in range(len(data))], data, marker='.', s=2)
# plt.xlabel("Frame number", fontsize=fs)
# plt.ylabel("Noise value", fontsize=fs)
# plt.show()

import numpy as np
import matplotlib.pyplot as plt

counts = [ 100, 200, 400, 800, 1600, 3200 ]
times = {}

path = "data/particleCountSweep"
fs = 18

for count in counts:
    times[count] = np.loadtxt(f"{path}/{count}", skiprows=0)

avg_times = [ np.mean(time) for time in times.values() ]
std_times = [ np.std(time) for time in times.values() ]

plt.errorbar(counts, avg_times, std_times, marker='.', ls = '', c = 'tab:red', capsize=4, ms=8)

plt.xlabel("Particle Counts", fontsize=fs)
plt.ylabel("Mean draw GL_TIME_ELAPSED", fontsize=fs)

plt.show()

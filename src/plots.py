import numpy as np
import matplotlib.pyplot as plt

fs = 18
cmap = plt.colormaps['hsv']

# data = np.loadtxt("data/v_order_param_raw.txt", delimiter=',', skiprows=1)
# data_lim = np.loadtxt("data/v_order_param_limited.txt", delimiter=',', skiprows=1)

# plt.plot(data[:,0], data[:,1], label="No acceleration limit")
# plt.plot(data_lim[:,0], data_lim[:,1], label="Limited acceleration")

# plt.legend()

# plt.title("Vicsek model velocity order parameter", fontsize=fs)



data = {}

NNList = [ 1, 2, 4, 8, 16, 24 ]

colors = cmap(np.linspace(0, 1, len(NNList)+1))
colors[:, :3] *= 0.85
path = "data/NNSweep"

for i in range(len(NNList)):
    data = np.loadtxt(f"{path}/order_param_NN_{NNList[i]}.txt", delimiter=',', skiprows=0)
    plt.plot(data[:,0], data[:,1], label=NNList[i], c=colors[i])

leg = plt.legend(fontsize=10, title="Nearest neighbors")
for line in leg.get_lines():
    line.set_linewidth(2.0)

plt.title("NN velocity order parameter", fontsize=fs)


plt.xlabel("Noise", fontsize=fs)
plt.ylabel(r"$\Phi$", fontsize=fs)

# plt.savefig('./figs/NN_v_order_param.pdf', dpi=500, bbox_inches='tight')


plt.show()


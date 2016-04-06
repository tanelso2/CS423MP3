import numpy as np
import matplotlib.pyplot as plt
np.set_printoptions(linewidth=100)


#data = np.genfromtxt("profile1.data",delimiter=" ", dtype=None)
#print data

import numpy as np
import matplotlib.pyplot as plt
  
lines = tuple(open("data.txt", 'r'))
data = []
for line in lines:
	data.append(line.split(' '))

del data[-1] 
accum_value = 0
page_fault_accum = []
for entry in data:
	entry[0] = int(entry[0])
 	entry[1] = int(entry[1])
 	entry[2] = int(entry[2])
 	entry[3] = int(entry[3].rstrip())
	accum_value += entry[0] + entry[1]
	page_fault_accum.append(accum_value)	

data_np = np.array(data)
print data_np


plt.plot(data_np[:,3], page_fault_accum)
plt.savefig("profile1_graph.png")


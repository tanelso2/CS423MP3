import numpy as np
import matplotlib.pyplot as plt
np.set_printoptions(linewidth=100)


##### CASE 2   
# MULTI 1
lines = tuple(open("multi1.txt", 'r'))
data = []
for line in lines:
	data.append(line.split(' '))

del data[-1] 
cpu_total = 0
for entry in data:
	entry[0] = int(entry[0])
 	entry[1] = int(entry[1])
 	entry[2] = int(entry[2])
 	entry[3] = int(entry[3].rstrip())
	cpu_total += entry[2]

cpu_1_avr = cpu_total / len(data)


# MULTI 5
lines = tuple(open("multi5.txt", 'r'))
data = []
for line in lines:
	data.append(line.split(' '))

del data[-1] 
cpu_total = 0
for entry in data:
	entry[0] = int(entry[0])
 	entry[1] = int(entry[1])
 	entry[2] = int(entry[2])
 	entry[3] = int(entry[3].rstrip())
	cpu_total += entry[2]

cpu_5_avr = cpu_total / len(data)



# MULTI 11
lines = tuple(open("multi11.txt", 'r'))
data = []
for line in lines:
	data.append(line.split(' '))

del data[-1] 
cpu_total = 0
for entry in data:
	entry[0] = int(entry[0])
 	entry[1] = int(entry[1])
 	entry[2] = int(entry[2])
 	entry[3] = int(entry[3].rstrip())
	cpu_total += entry[2]

cpu_11_avr = cpu_total / len(data)


# Graph Here 
bar_width = 0.35

cpu_averages = (cpu_1_avr, cpu_5_avr, cpu_11_avr)
index = np.arange(3)

plt.bar(index, cpu_averages, bar_width)

plt.xlabel('N Processes')
plt.ylabel('Average CPU Utilization')
plt.title('Average CPU Utilization vs. # of Work Processes')
plt.xticks(index + bar_width, ('N=1', 'N=5', 'N=11'))
plt.legend()

plt.tight_layout()
plt.savefig("multi_graph.png")





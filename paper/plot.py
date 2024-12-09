import matplotlib.pyplot as plt

# Read data from the log file
min_utilities = []
execution_times = []

with open("execution_log.txt", "r") as file:
    for line in file:
        parts = line.strip().split(", ")
        min_utility = float(parts[0].split(": ")[1])
        execution_time = float(parts[1].split(": ")[1].split()[0])
        min_utilities.append(min_utility)
        execution_times.append(execution_time)

# Zip the values together, sort them by min_utility, and unzip them back
data = sorted(zip(min_utilities, execution_times))
min_utilities, execution_times = zip(*data)

# Plot the data
plt.plot(min_utilities, execution_times, marker='o')
plt.xlabel("Min Utility")
plt.ylabel("Execution Time (seconds)")
plt.title("Execution Time vs Min Utility")
plt.grid(True)
plt.show()

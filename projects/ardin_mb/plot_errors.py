import re
import sys
import matplotlib.pyplot as plt

def read_errors(filename):
    # Open file
    errors = []
    with open(filename) as f:
        # Read alternate lines (error rates)
        for l in f.readlines()[1::2]:
            # Use regex to parse line
            counts = re.match("Destination reached in ([0-9]+) steps with ([0-9]+) errors", l.rstrip())
            #print int(counts.group(1)), int(counts.group(2))

            # Add number of errors to0
            errors.append(int(counts.group(2)))
    return errors

# Read errors
errors = read_errors(sys.argv[1])

fig, axis = plt.subplots()
axis.boxplot(errors, 0, '')
plt.show()
import matplotlib.pyplot as plt
import csv
import time
  

while(1):
    time.sleep(20)
    plt.rcParams["figure.figsize"] = [20, 10]
    plt.rcParams["figure.autolayout"] = True
    fig = plt.figure()
    x = []
    y = []
    with open('Power_State.csv','r') as csvfile:
        lines = csv.reader(csvfile, delimiter=',')
        for count,row in enumerate(reversed(list(lines))):
            if(count==0):
                continue
            if(count>20):
                break
            x.append(row[1])
            y.append(int(row[0]))
            
    x.reverse()
    y.reverse()
    plt.plot(x, y, color = 'g', linestyle = 'dashed',
            marker = 'o',label = "Power State Variation")

    plt.xticks(rotation =25)
    plt.xlabel('Time')
    plt.ylabel('Power State')
    plt.title('Led Strip Power State', fontsize = 20)
    plt.grid()
    plt.legend()
    fig.subplots_adjust(bottom=0.3)
    print('plot saved.\n')
    plt.savefig('Power.png')

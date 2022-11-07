import subprocess
import matplotlib.pyplot as plt
fig,axis=plt.subplots(2)
users=200
iterations=10
avgerage_RT=[]
# noofuser=list()
# throughput=list()
noofuser=[]
throughput=[]


for i in range(iterations):
    test = subprocess.Popen(["./loadgen","localhost","8082",str(users),"0.1,","40"], stdout=subprocess.PIPE)
    output = test.communicate()[0]
    decodedOutput=output.decode("ascii")
    print(decodedOutput)
    
    noofuser.append(users)
    avgerage_RT.append(float(decodedOutput.split("\n")[7].split(":")[1].strip()))
    throughput.append(float(decodedOutput.split("\n")[8].split(":")[1].strip()))
    
    print(throughput)

    axis[1].plot(noofuser,avgerage_RT)
    axis[1].set_title("Average Round Trip")
    axis[1].set_ylabel("Avg Response Time")
    axis[1].set_xlabel("No. of Users")
    axis[0].plot(noofuser,throughput)
    axis[0].set_title("Throughput")
    axis[0].set_xlabel("No. of Users")
    axis[0].set_ylabel("Throughput")
    plt.tight_layout()
    plt.savefig("output_graph.png")

    users+=200
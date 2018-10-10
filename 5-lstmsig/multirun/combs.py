import subprocess, sys, six, threading,os, time, itertools, os.path, platform, pydoc
import queuing
from six.moves import input
from six import print_

levels = [1,2,3,4]
hiddens = [10,20,30,40,50,60]
dims = [10,20,30,40,50,60]
hiddens = [10,30,50]
dims = [10,30,50]

#extralength=0
#midlevelwidth=40
#problems = [(40,15,0),(20,15,0)]#total,seq,extra
problems = [(20,15,0)]

p=queuing.ParameterSet([problems,levels,dims,hiddens])

def nParams(paramset):
    problem=paramset[0]
    level=paramset[1]
    dim=paramset[2]
    hidden=paramset[3]
    inputdim=1
    siglength=sum([dim**l for l in range(1,level+1)])
    return (3*dim+hidden)*(1+inputdim+hidden)+hidden*(1+siglength)

if len(sys.argv)>1: #control actions
    if input("refill queue? (y/n)") == "y":
        print("ok")
        queuing.fill_queue(p)
        sys.exit()
    if input("print tasks? (y/n)") == "y":
        s=[]
        for i,c in enumerate(p.all_combinations):
            s.append(str((i,c,nParams(c))))
        s = "\n".join(s)
        pydoc.pager(s)
        #for i in enumerate(p.all_combinations):
            #print_(i,end=";")
            #print_(i)
        #print_()
    sys.exit()

if platform.uname()[1]=="fifi":
    print("you WHAT?") #just in case I run this script with no arguments on my machine
    sys.exit()
    
pythonCmd = "python3"
pythonScript = "isRandom.py"

while 1:
    if os.path.isfile('stop.txt'):
        print ("exiting "+__file__+" at "+ time.strftime("%Y-%m-%d %H:%M:%S"))
        sys.exit()
    task=queuing.get_from_queue(p)
    if task is None:
        break
    print(task)
    (prob,level,dim,hidden) = task
    env = os.environ.copy()
    env["JR_DATALENGTH"]=str(prob[0])
    env["JR_DATAKNOWNLENGTH"]=str(prob[1])
    env["JR_DATAEXTRALENGTH"]=str(prob[2])
    env["JR_LEVEL"]=str(level)
    env["JR_SIGDIM"]=str(dim)
    env["JR_NHIDDEN"]=str(hidden)
    subprocess.call([pythonCmd,pythonScript],env=env)

print ("exiting "+__file__+" because done at "+ time.strftime("%Y-%m-%d %H:%M:%S"))


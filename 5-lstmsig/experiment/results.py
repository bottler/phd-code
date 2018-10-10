import json
import os
import numpy,pydoc,itertools,difflib
import datetime, time
import tabulate
#import JFRmail
import oncampus
from six import print_
graphToFile = not oncampus.draw
if graphToFile:
    import matplotlib
    matplotlib.use("pdf")
filename = "results.sqlite"
if "JR_RESULTSFILE" in os.environ:
    filename=os.environ["JR_RESULTSFILE"]
#useCPP is an alternative to python's sqlite3 library which is only needed on tinis.
useCPP = ("JR_NO_SQLITE3" in os.environ)
if useCPP:
    import report
    report.open(filename)
else:
    import sqlite3
    con = sqlite3.connect(filename)
nrun = 0
nsteps = 0
starttime = 0
oldColumns = True

#todo - be flexible about the results column
#     - keep a global variable of their names, prettyname, bigIsGood


def startRun(continuation, attribs, architecture, solver, callerFilePath):
    global nrun
    global filecontents
    filecontents=""
    if callerFilePath is not None:
        def getC(f):
            global filecontents
            with open(f,"r") as ff:
                filecontents=filecontents+f+"\n"+ff.read()
        if(type(callerFilePath)==tuple):
            for f in callerFilePath:
                getC(f)
        else:
            getC(callerFilePath)
    runColList="(COUNT INTEGER PRIMARY KEY, TIME TEXT DEFAULT CURRENT_TIMESTAMP NOT NULL, REPN TEXT, REPNLENGTH INT, CONTINUATION TEXT, BATCHTR INT, BATCHTE INT, LAYERTYPE TEXT, LAYERS INT, WIDTH INT, ARCHITECTURE TEXT, SOLVER TEXT, CODE TEXT)"
    if not oldColumns:
        runColList="(COUNT INTEGER PRIMARY KEY, TIME TEXT DEFAULT CURRENT_TIMESTAMP NOT NULL, CONTINUATION TEXT, ARCHITECTURE TEXT, SOLVER TEXT, CODE TEXT)"
    setup=["create table if not exists RUNS"+runColList,#these default keys start at 1
           "create table if not exists STEPS(STEP INTEGER PRIMARY KEY, RUN int, OBJECTIVE real, TRAINACC real, TESTOBJECTIVE real, TESTACC REAL )",
           "create table if not exists TIMES(RUN INT, TIME real)", # stores the time of the tenth step of each run, allowing speed to be measured
           "create table if not exists ATTRIBS(RUN INT, NAME TEXT, ISRESULT INT, VALUE)" #value deliberately has no affinity. some are numbers
       ]
    infoquery = "insert into RUNS (CONTINUATION, ARCHITECTURE, SOLVER, CODE) VALUES (?,?,?,?)"
    info = (continuation, architecture, solver, filecontents)
    attribquery = "insert into ATTRIBS(RUN, NAME, ISRESULT, VALUE) VALUES (?,?,0,?)"
    if useCPP:
        for s in setup:
            report.resultless(s)
        report.sink(infoquery,info)
        nrun = report.lastRow()
        for k,v in sorted(attribs.items()):
            report.sink(attribquery,(nrun,k,v))
    else:
        c = con.cursor()
        for s in setup:
            c.execute(s)
        c.execute(infoquery, info)
        nrun = c.lastrowid
        c.executemany(attribquery,[([nrun]+list(i)) for i in sorted(attribs.items())])
        con.commit()

#store elapsed time with each step?
#We store the time taken for 10 steps, but actually we count steps 2 to 11, because if you use
#keras the first step may include some of the compilation
def step(obj, train, objte, test):
    global nsteps, starttime
    q = [("insert into steps values (NULL, ?, ?, ?, ?, ?)",(nrun,obj,train,objte,test))]
    nsteps = 1 + nsteps
    if nsteps == 1:
        starttime = datetime.datetime.now()
    if nsteps == 11:
        #c.execute("insert into TIMES (RUN) VALUES (?)", (nrun,))
        q.append(("insert into TIMES VALUES (?,?)", (nrun, (datetime.datetime.now()-starttime).total_seconds())))

    if useCPP:
        for sql, data in q:
            report.sink(sql,data)
    else:
        c=con.cursor()
        for sql, data in q:
            c.execute(sql,data)
        con.commit()  

def addResultAttribute(name, value):
    q=[("insert into ATTRIBS(RUN, NAME, ISRESULT, VALUE) VALUES (?,?,1,?)",(nrun, name, value))]
    c=con.cursor()
    if True:
        count = c.execute("select count(*) from ATTRIBS where run = ? and name = ?",(nrun,name)).fetchone()[0]
        if count>0:
            raise RuntimeError("attribute already set:",name)
    for sql, data in q:
        c.execute(sql,data)
    con.commit()     
        
def signoff():
    con.close()
    
    
#here begin analysis functions

def printJSON_nicely(j):
    #print json.dumps(json.loads(j), sort_keys=True,indent=4, separators=(',', ': '))
    return json.dumps(json.loads(j), indent=4, separators=(',', ': '))

def movingAverage(a, padToSameLength, n=10, padValue=0):
    b = numpy.cumsum(a)
    b = numpy.insert(b,0,0) #was missing
    c = (b[n:]-b[:-n])/n
    #c now has length len(a)+1-n
    if padToSameLength and n>1:
        return numpy.concatenate([numpy.repeat(padValue,n-1),c])
    return c

def replaceSeriesWithMovingAverage_(z, avgLength, highIsBad):
    z=numpy.array(z,dtype="float64")
    badValue = numpy.amax(numpy.nan_to_num(z)) if highIsBad else 0
    z[numpy.isnan(z)]=badValue
    z=movingAverage(z, True, avgLength, padValue=badValue)
    return z

movingAvgLength = 10

#the pyplot module is taken as a parameter here as it is only imported when asked for
def pushplot(plt,title=""):
    if graphToFile:
        plt.savefig("/home/jeremyr/Dropbox/phd/graphs/"+time.strftime("%Y%m%d-%H%M%S")+title)
        plt.close()
    else:
        multiwindow=False
        if multiwindow:#multiple graphs, non blocking
            plt.show(block=False)
            plt.figure()
        else:
            plt.show()
 
def runList():
    c=con.cursor()
    a=c.execute("select * from RUNS").fetchall()
    for j in a:
        #print j
#        if j[0]<10:
#            print "",
        print (j[:-3]) # to omit the long json descriptions
    b=c.execute("select count, case when length(continuation) >0 then 'contd' else '' end, count(step), min(step), min(objective), max(trainacc), min(testobjective), max(testacc), case when count(objective)=count(*) then '' else 'nan' end from runs r left join steps s on r.count=s.run group by count").fetchall()
    print ( tabulate.tabulate([i for i in b],headers=["","","steps", "start","obj","trainAcc","testObj","testAcc","Bad"]) )
    #return a,b

def _getUseOldColumns():
    return oldColumns and con.execute("select sql like '%LAYERTYPE%' from sqlite_master where tbl_name='RUNS'").fetchone()[0]
    
def runList1():
    c=con.cursor()
    if not _getUseOldColumns():
        raise RuntimeError("runList1 not available in the new format")
    b=c.execute("select repnlength, repn, count(step), min(objective), max(trainacc), max(testacc) from runs r left join steps s on r.count=s.run group by count").fetchall()
    for j in b:
        print(j)

def _listAttribs(wantRes):
    c=con.cursor()
    if wantRes is None:
        a = c.execute("select distinct(name) from ATTRIBS").fetchall()
    else:
        a = c.execute("select distinct(name) from ATTRIBS where ISRESULT = ?", (wantRes,)).fetchall()
    b=sorted([i[0] for i in a])
    return b

def _getAttribAsStr(run,name):
    c=con.cursor()
    a = c.execute("select value from ATTRIBS where run = ? and name = ?",(run,name)).fetchall()
    for i in a:#there should only be one if there are any
        return str(i[0])
    return ""
    
#architectureLike: if set to 5, only print info for runs with the same network as the one used in run 5
#doAttribs: None means all attribs are included, a list of strings means those attributes are included,
#   a list containing a list of strings means those attributes are excluded
#   True or False mean only the result or nonresult ones.
defaultAttribs = None
defaultAttribs = [["dataextrapoints", "type"]]
def runList2(avgLength=None, doPrint = True, runFrom=None, architectureLike=None, doAttribs=defaultAttribs): #with times
    if type(doAttribs)==list:
        if len(doAttribs)==1 and type(doAttribs[0]==list):
            attribs = [i for i in _listAttribs(None) if i not in doAttribs[0]]
        else:
            attribs = doAttribs
    else:
        attribs =  _listAttribs(doAttribs)
    if avgLength == None:
        avgLength = movingAvgLength
    c=con.cursor()
    b=[]
    archClause = ""
    masterQueryArgs = None
    useOldColumns=_getUseOldColumns()
    if architectureLike is not None:
        if not useOldColumns:
            raise RuntimeError("no architecture search with new format")
#        architectureStr = c.execute("select architecture from runs where count = ?",(architectureLike,)).fetchone()[0]
#        archClause = "where architecture = ?"
#        masterQueryArgs = (architectureStr,)
        masterQueryArgs = c.execute("select LAYERS, LAYERTYPE, WIDTH from runs where count = ?",(architectureLike,)).fetchone()
        archClause = "where layers = ? and layertype = ? and width = ?"
        if runFrom is not None:
            archClause = archClause + " and count >= " + str(runFrom)
            runFrom = None
    masterQuery = "select count, "+("repnlength, repn," if useOldColumns else "") +" case when length(continuation)>0 then '+' else '' end || count(step), (select time from times where run = r.count) from runs r left join steps s on r.count=s.run %s group by count" % (archClause if runFrom is None else ("where count>= "+str(runFrom)))
    if masterQueryArgs is None:
        bb=c.execute(masterQuery).fetchall()
    else:
        bb=c.execute(masterQuery,masterQueryArgs).fetchall()
    for rec in bb:
        values = c.execute("select objective,trainacc,testacc from steps where run=? order by step",(rec[0],))
        values = [i for i in values]
        def bestAvg(idx,highIsBad):
            if len(values)<avgLength:
                return None
            s=replaceSeriesWithMovingAverage_([i[idx] for i in values],avgLength,highIsBad)
            return (numpy.amin if highIsBad else numpy.amax)(s)
        attribValues=tuple(_getAttribAsStr(rec[0],att) for att in attribs)
        b.append(rec[:-1] + (bestAvg(0,True),bestAvg(1,False),bestAvg(2,False),rec[-1])+attribValues)
    if doPrint:
        if useOldColumns:
            headers=["","repLen","repn","steps","objective","trainAcc","testAcc","10StepTime"]+attribs
        else:
            headers=["",                "steps","objective","trainAcc","testAcc","10StepTime"]+attribs
        print (tabulate.tabulate(b,headers=headers))
    else:
        return b

def diffRuns(x,y, pretty=1):
    c=con.cursor()
    q=c.execute("select * from runs where count = ?", (x,))
    xx=list(q.fetchone())
    yy=list(c.execute("select * from runs where count = ?", (y,)).fetchone())
    #print len(xx), len(yy)
    for i in range(3):
        x1 = xx.pop()
        y1 = yy.pop()
        if x1!=y1 and x1 is not None and y1 is not None:
            if pretty:
                if i>0: #so data is json
                    x1=printJSON_nicely(x1).split("\n")
                    y1=printJSON_nicely(y1).split("\n")
                    x1=["."+x for x in x1]#this helps with alignment in tabulate
                    y1=["."+x for x in y1]
                else:
                    x1=[] if x1 is None else x1.split("\n")
                    y1=[] if y1 is None else y1.split("\n")
                pydoc.pager("\n".join(difflib.Differ().compare(x1,y1)))
                #pydoc.pager(tabulate.tabulate([x for x in itertools.izip_longest(x1,y1,fillvalue="")]))
            else:
                print (x1)
                print (y1)
    dif = [(d[0],a,b) for (a,b,d) in zip(xx,yy,q.description) if (a!=b)]
    xxx = [i for i in c.execute("select name,value from attribs where run = ?", (x,)).fetchall()]
    yyy = [i for i in c.execute("select name,value from attribs where run = ?", (y,)).fetchall()]
    labx= [k for k,v in xxx]
    laby= [k for k,v in yyy]
    dx=dict(xxx)
    dy=dict(yyy)
    shared=[i for i in labx if i in laby]
    dif=(dif+[(i,dx[i],dy[i]) for i in shared if dx[i]!=dy[i]]+[(i,"",dy[i]) for i in laby if i not in shared]+
         [(i,dx[i],"") for i in labx if i not in shared])
    print (tabulate.tabulate(dif))

def describeRun(run):
    c=con.cursor()
    q=c.execute("select * from runs where count = ?", (run,))
    xx=list(q.fetchone())
    for i in range(3):
        x1 = xx.pop()
        if x1 is not None:
            if i>0:
                x1=printJSON_nicely(x1)
            pydoc.pager(x1)
    data = [(d[0],a) for a,d in zip(xx,q.description)]
    data2=c.execute("select name,value from attribs where run = ?", (run,)).fetchall()
    print (tabulate.tabulate(data+[i for i in data2]))
    

def lastSteps(_pager=False, _rtn=False):
    c=con.cursor()
    a=c.execute("select * from STEPS where run = (select max(count) from runs) order by step").fetchall()
    text = tabulate.tabulate([(ii,)+i[2:] for ii,i in enumerate(a)])
    if _rtn:
        return [i[2:] for i in a]
    if _pager:
        pydoc.pager(text)
    else:
        print (text)

def steps(run, doPrint):
    c=con.cursor()
    a=c.execute("select * from STEPS where run = ? order by step",(run,)).fetchall()
    if doPrint:
        print (tabulate.tabulate([(ii,)+i[2:] for ii,i in enumerate(a)]))
    else:
        return a

def gotnan(run):
    c=con.cursor()
    a=c.execute("select objective from STEPS where step = (select max(step) from steps where run = ?)",(run,)).fetchone()
    return a[0] is None

def l(): #prints the most important things to know about the current long-running job - is it making progress
    c=con.cursor()
    a=c.execute("select trainacc,testacc,objective,testobjective from STEPS where run = (select max(count) from runs) order by step").fetchall()
    tra=[w for (w,x,y,z) in a]
    tea=[x for (w,x,y,z) in a]
    tro=[y for (w,x,y,z) in a]
    teo=[z for (w,x,y,z) in a]
#    print numpy.argmin(tro)
#    print len(a)
    def v(name, t, ismin):
        ar = numpy.argmin if ismin else numpy.argmax
        m = numpy.min if ismin else numpy.max
        #we could take more care of nans here
        r = lambda x : round(x,4) if x is not None else "NAN?"
        return (name,r(m(t)),r(ar(t)/(len(a)-1.0)),r(m(t[:len(a)//2])))
    teo_=replaceSeriesWithMovingAverage_(teo,movingAvgLength,True)
    tea_=replaceSeriesWithMovingAverage_(tea,movingAvgLength,False)   
    d=[v("trainObj",tro,1),
       v("trainAcc",tra,0),
       v( "testObj",teo,1),
       v( "testAcc",tea,0),
       v( "testObj_Avg",teo_,1),
       v( "testAcc_Avg",tea_,0)]
    print (tabulate.tabulate(d, headers=("","best","bestIsWhere","firstHalf")))

#could provide more here
def runInfo(run):
    c=con.cursor()
    a=c.execute("select objective, trainacc, testobjective, testacc from STEPS where run = ? order by step", (run,)).fetchall()
    return a

def addColumn(name):
    c=con.cursor()
    #c.execute("ALTER TABLE RUNS ADD COLUMN ? TEXT", (name,))
    c.execute("ALTER TABLE RUNS ADD COLUMN "+ name +" TEXT")
    con.commit()

def deleteRun(run):
    c=con.cursor()
    c.execute("delete from STEPS where run = ?", (run,))
    c.execute("delete from TIMES where run = ?", (run,))
    c.execute("delete from RUNS where count = ?", (run,))
    c.execute("delete from ATTRIBS where run = ?", (run,))
    con.commit()

def _runToStr(run):
    if isinstance(run,(tuple,list)):
        return ""
    return str(run)
    
def plotRun(run, runningMax=False, objective=False, testAvg=10, trainAvg=1):
    c=con.cursor()
    query = "select trainacc, testacc from STEPS where run = ? order by step"
    if objective:
        query = "select objective, testobjective from STEPS where run = ? order by step"
    if isinstance(run,(tuple,list)):
        a=[i for r in run for i in c.execute(query, (r,)).fetchall()]
    else:
        a=c.execute(query, (run,)).fetchall()
    import matplotlib.pyplot as plt
    x=range(len(a))
    y=[Y for (Y,Z) in a]
    z=[Z for (Y,Z) in a]

    if (not runningMax) and (testAvg is not None) and testAvg>1 and len(y)>3*testAvg:
        z=replaceSeriesWithMovingAverage_(z,testAvg,objective)
    if (not runningMax) and (trainAvg is not None) and trainAvg>1 and len(y)>3*trainAvg:
        y=replaceSeriesWithMovingAverage_(y,trainAvg,objective)
        
    if runningMax:#True:
        if objective:
            y=numpy.minimum.accumulate(y)
            z=numpy.minimum.accumulate(z)
        else:
            y=numpy.maximum.accumulate(y)
            z=numpy.maximum.accumulate(z)
    plt.plot(x,y,"b",x,z,"r+-")
    n=("O" if objective else "_")+_runToStr(run)
    pushplot(plt,n)

def plotRuns(runx, runy, runningMax=False, test=True, extend=True, objective=False, testAvg=10):
    if objective:
        column = ("testobjective" if test else "objective")
    else:
        column = ("testacc" if test else "trainacc")
    c=con.cursor()
    def getRun(run):
        query = "select " + column + " from STEPS where run = ? order by step"
        if(isinstance(run,(tuple,list))):
            return [j for r in run for j in getRun(r)]
        return c.execute(query, (run,)).fetchall()
    y=getRun(runx)
    z=getRun(runy)
    import matplotlib.pyplot as plt
    l = min(len(z),len(y))
    x1=range(len(y))
    x2=range(len(z))
    y=[Y for (Y,) in y]
    z=[Y for (Y,) in z]
    if (not runningMax) and test and (testAvg is not None) and testAvg>1 and l>3*testAvg:
        y=replaceSeriesWithMovingAverage_(y,testAvg,objective)
        z=replaceSeriesWithMovingAverage_(z,testAvg,objective)
    if not extend:
        x1=x2=range(l)
        y=y[:l]
        z=z[:l]
    if runningMax:#True:
        y=numpy.maximum.accumulate(y)
        z=numpy.maximum.accumulate(z)
    plt.plot(x1,y,"b",x2,z,"r")
    n=("Te" if test else "Tr")+("O" if objective else "")+_runToStr(runx)+"vs"+_runToStr(runy)
    pushplot(plt, n)

#like plotRuns but takes a list of runs, so can to arbitrarily many
def plotRunss(runs, runningMax=False, test=True, extend=True, objective=False, testAvg=10):
    if objective:
        column = ("testobjective" if test else "objective")
    else:
        column = ("testacc" if test else "trainacc")
    c=con.cursor()
    def getRun(run):
        query = "select " + column + " from STEPS where run = ? order by step"
        if(isinstance(run,(tuple,list))):
            return [j for r in run for j in getRun(r)]
        return c.execute(query, (run,)).fetchall()
    y = [getRun(i) for i in runs]
    y = [i for i in y if len(i)>0]
    import matplotlib.pyplot as plt
    l = numpy.min([len(i) for i in y])
    x = [range(len(i)) for i in y]
    y=[[Y for (Y,) in i] for i in y]
    if (not runningMax) and test and (testAvg is not None) and testAvg>1 and l>3*testAvg:
        y=[replaceSeriesWithMovingAverage_(i,testAvg,objective) for i in y]
    if not extend:
        x=[range(l) for i in y]
        y=[i[:l] for i in y]
    if runningMax:#True:
        y=[numpy.maximum.accumulate(i) for i in y]
    a = [ j for i in zip (x,y,itertools.cycle(("b","r","y","k"))) for j in i]
    plt.plot(*a)
    pushplot(plt)

#This is a text-only way to compare two run performances - equivalent of plotRuns, 
def rrs(runx, runy, test=True, extend=True):
    c=con.cursor()
    def getRun(run):
        query = "select " + ("testacc" if test else "trainacc") + " from STEPS where run = ? order by step"
        if(isinstance(run,(tuple,list))):
            return [j for r in run for j in getRun(r)]
        return c.execute(query, (run,)).fetchall()
    y=getRun(runx)
    z=getRun(runy)
    def isbest(xx): ##returns vector with length of xx, with 1 at max-so-far places
        out = [0]*len(xx)
        best = -1
        for i,j in enumerate(xx):
            if j>best:
                best=j
                out[i]=1
        return out        
    y=[Y for (Y,) in y]
    z=[Y for (Y,) in z]
    ym=numpy.maximum.accumulate(y)
    zm=numpy.maximum.accumulate(z)
    yb=isbest(y)
    zb=isbest(z)
    o = [(i,y[i] if yb[i] else "" ,"-->" if ym[i]<zm[i] else "<--", z[i] if zb[i] else "") 
         for i in range(min(len(y),len(z))) if (yb[i] or zb[i])]
    if extend:
        if len(z)>len(y):
            o.extend([(i,".","",z[i]) for i in range(len(y),len(z)) if zb[i]])
        if len(y)>len(z):
            o.extend([(i,y[i],"",".") for i in range(len(z),len(y)) if yb[i]])
#    print tabulate.tabulate(o,headers=["step",runx,runy])
    pydoc.pager(tabulate.tabulate(o,headers=["step",runx,"best",runy]))
    
if False:
    from incs import *
    l = lastBads();
    plt.ion();
    plt.show();
    for (val,thou,tru) in l:
        print_ ("thought ",thou," but is really a ", tru, "  (",val,",",testY[val],")")
        #draw(testX[val][0])
        plt.clf()
        plt.plot(testX[val][0][:,0],testX[val][0][:,1])
        plt.draw()
        raw_input("foo")

    print ("ok")

#can do c.execute("select count(*) from strokes natural join trainx group by trainxn").fetchall()


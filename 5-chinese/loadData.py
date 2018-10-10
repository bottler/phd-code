#Ben Graham and Jeremy Reizenstein, University of Warwick, GPL v3
import numpy, manipulateCharacters
import multiprocessing, sys, os, six, shapes
from six.moves import range
if six.PY3:
    #import baresig3 as baresig
    from baresig3 import sig
else:
    from baresig import sig
#import embed

#encoding="strokeIncrementsB"
#encoding="pointConcatenatedDir"
encoding="localSigsPen"
#encoding="strokeSig"
nClasses=3755
pyramid=False #if True (set below) then transform returns a list of arrays not an array
oneHotLabels = False #if true, old keras style, where y is supplied one hot

def JoinPenDim(char):
    joins=[numpy.array([[char[i][-1,0],char[i][-1,1],1],[char[i+1][0,0],char[i+1][0,1],1]],dtype='float32') for i in range(len(char)-1)]
    char=[numpy.hstack([stroke,numpy.zeros((stroke.shape[0],1),dtype='float32')]) for stroke in char]
    char=[char[i//2] if i%2==0 else joins[i//2] for i in range(len(char)*2-1)]
    char=numpy.vstack(char)
    return char
def JoinWithSignalForNewStroke(char):
    a=numpy.vstack([numpy.vstack([numpy.hstack([numpy.ones((x.shape[0],1),dtype='float32'),x]),numpy.zeros((1,3),dtype='float32')]) for x in char])
#    return a
    return a[:-1,:]

if encoding=="strokeSig":
    level=3
    def transform(char):
        c1=[i[0,:] for i in char]
        c2=[sig(i,level) for i in char]
        c = [numpy.concatenate(i) for i in zip(c1,c2)]
        d = numpy.vstack(c).astype("float32")
        return d
    encodingString="strokeSig"+str(level)
        

if encoding=="point":
    #all the points along each stroke, with a 1, concatenated, but with a zero row between 
    def transform(char):
        a=manipulateCharacters.constantSpeedCharacter(char, 0.4)#was .3
        a=numpy.vstack([numpy.vstack([numpy.hstack([numpy.ones((x.shape[0],1),dtype='float32'),x]),numpy.zeros((1,3),dtype='float32')]) for x in a])
        return a

if encoding=="pointConcatenated":
    #all the points along the concatenation of the strokes - so lifting the pen or drawing a straight line will look identical
    length_ = 0.5
    def transform(char):
        a=manipulateCharacters.constantSpeed(numpy.vstack(char), length_)
        return a
    encodingString = "pointConcatenated" + str(length_)
if encoding=="pointWithPenFailed":
    def transform(char):
        joins=[numpy.array([[char[i][-1,0],char[i][-1,1],0],[char[i+1][0,0],char[i+1][0,1],0]],dtype='float32') for i in range(len(char)-1)]
        char=[numpy.hstack([stroke,numpy.zeros((stroke.shape[0],1),dtype='float32')]) for stroke in char]
        char=[char[i//2] if i%2==0 else joins[i//2] for i in range(len(char)*2-1)]
        char=numpy.vstack(char)
        a=manipulateCharacters.constantSpeedLookingAtFirstTwoDims(char, 0.3)
        return a
if encoding=="pointWithPen":
    distance=0.3
    def transform(char):
        char=JoinPenDim(char)
        a=manipulateCharacters.constantSpeedLookingAtFirstTwoDims(char, distance)
        return a
    encodingString="pointWithPenCorrected" + str(distance)

#include cos and sin of the dir at the point
if encoding=="pointConcatenatedDir":
    distance=0.3
    def transform(char):
        char=numpy.vstack(char)
        a=manipulateCharacters.constantSpeedLookingAtFirstTwoDimsWithDir(char, distance)
        return a
    encodingString="pointConcatenatedDir" + str(distance)
if encoding=="pointWithPenDir":
    distance=0.5
    def transform(char):
        char=JoinPenDim(char)
        a=manipulateCharacters.constantSpeedLookingAtFirstTwoDimsWithDir(char, distance)
        return a
    encodingString="pointWithPenDir" + str(distance)

if encoding=="stroke":
    strokelen = 5
    def transform(char):
        a=manipulateCharacters.fixedLengthStrokesCharacter(char, strokelen)
        a=[x.flatten()[None,:] for x in a]
        a=numpy.vstack(a)
        return a
    encodingString="stroke" + str(strokelen)
if encoding=="strokeSubtractStartPointsAndStoreSeparately":
    1#(x0,y0),(x1,y1),...,(xn,yn) -> (x0,y0), (x1-x0,y1-y0), ... (xn-x0,yn-y0) -> flatten
if encoding=="strokeIncrements":
    #(x0,y0),(x1,y1),...,(xn,yn) -> (x1-x0,y1-y0), ... (xn-x{n-1},yn-y{n-1}), (x0,y0) -> flatten
    s_i_length = 15
    def transform(char):
        a=manipulateCharacters.fixedLengthStrokesCharacter(char, s_i_length)
        a=numpy.vstack([numpy.concatenate([3*(x[:-1]-x[1:]).flatten(),x.mean(0)]).reshape((1,-1)) for x in a])
        return a
    encodingString = "strokeIncrements"+str(s_i_length)
if encoding=="strokeIncrementsB":
    #(x0,y0),(x1,y1),...,(xn,yn) -> (x1-x0,y1-y0), ... (xn-x{n-1},yn-y{n-1}), (x0,y0) -> flatten
    s_ib_length = 15
    def transform(char):
        a=manipulateCharacters.fixedLengthStrokesCharacter(char, s_ib_length)
        a=numpy.vstack([numpy.concatenate([
            3*(x[:-1]-x[1:]).flatten(),
            numpy.clip((x[:,0].mean()+1)*8-numpy.arange(1,2*8,2),-1,1),
            numpy.clip((x[:,1].mean()+1)*8-numpy.arange(1,2*8,2),-1,1),
            ]).reshape((1,-1)) for x in a])
        return a
    encodingString = "strokeIncrementsB"+str(s_ib_length)


def starts(length, seqlength, overlap):
    core = [i for i in range(0,length-seqlength+1, seqlength-overlap)]
    if core[-1]!= length-seqlength:
        core.append(length-seqlength)
    return core

#arr is a 2D array which we split along axis 0 into segments of length seqlength
def segments(arr, seqlength, overlap):
    if arr.shape[0]<seqlength:
        return [arr]
    o=[]
    for i in starts(arr.shape[0],seqlength,overlap):
        o.append(arr[i:(i+seqlength)])
    return o

def segmentList(l, seqlength):
    if len(l)<seqlength:
        return [l]
    o=[]
    for i in starts(len(l),seqlength,0):
        o.append(l[i:(i+seqlength)])
    return o    

if encoding =="localSigsConcatenated":
    minilength = 0.05
    seqlength = 6 #multiple of minilength
    overlap = 0
    level = 4
    def transform(char):
        char=numpy.vstack(char)
        a = manipulateCharacters.constantSpeed(char,minilength)
        b = segments (a, seqlength, overlap)
#        c = [embed.sig_notemplates(i.astype("float32"), level) for i in b]
        c = [sig(i.astype("float32"), level) for i in b]
        c1 = [i[0,:] for i in b]
        c = [numpy.concatenate(i) for i in zip(c,c1)]
        d = numpy.vstack(c)
        return d
    #the L means location information was added
    encodingString = "localSigsConcatenatedL" + str(level)+"_"+str(minilength)+"_"+str(overlap)+"_"+str(seqlength)

if encoding =="localSigsPen":
    minilength = 0.04
    seqlength = 20 #multiple of minilength
    overlap = 0
    level = 0
    sig2dOnly = False
    speedLooksAtFirstTwoDims=False#Bad idea True
    boundingBox = False
    centroid = False
    def transform(char):
        char=JoinPenDim(char)
        a = manipulateCharacters.constantSpeedLookingAtFirstTwoDims(char,minilength) if speedLooksAtFirstTwoDims else manipulateCharacters.constantSpeed(char,minilength) 
        b = segments (a, seqlength, overlap)
        sigand = [(i[:,:2] if sig2dOnly else i).astype("float32") for i in b]
        c = [sig(i, level) for i in sigand] if level > 0 else [[] for i in b]
        c1 = [i[0,:] for i in b]
        c2 = [numpy.amax(i[:,:2],0) for i in b] if boundingBox else [[] for i in b]
        c3 = [numpy.amin(i[:,:2],0) for i in b] if boundingBox else [[] for i in b]
        c4 = [numpy.mean(i[:,:2],0) for i in b] if centroid else [[] for i in b]
        c = [numpy.concatenate(i) for i in zip(c,c1,c2,c3,c4)]
        d = numpy.vstack(c)
        return d
    encodingString = "localSigsPen" + ("*" if speedLooksAtFirstTwoDims else "") + str(level)+"_"+str(minilength)+"_"+str(overlap)+"_"+str(seqlength)+("_2d" if sig2dOnly else "")+str("bb" if boundingBox else "")+str("ctr" if centroid else "")

if encoding =="localSigsPyramid":
    minilength = 0.04
    seqlength = 20 #multiple of minilength
    overlap = 0
    level = 3
    sig2dOnly = True
    speedLooksAtFirstTwoDims=False# Bad idea True
    boundingBox = False
    centroid = False
    higherSigs = [(4,4)]#(number to merge, level)
    #here transform returns multiple arrays
    def transform(char):
        char=JoinPenDim(char)
        a = manipulateCharacters.constantSpeedLookingAtFirstTwoDims(char,minilength) if speedLooksAtFirstTwoDims else manipulateCharacters.constantSpeed(char,minilength) 
        b = segments (a, seqlength, overlap)
        sigand = [(i[:,:2] if sig2dOnly else i).astype("float32") for i in b]
        c = [sig(i, level) for i in sigand]
        c1 = [i[0,:] for i in b]
        c2 = [numpy.amax(i[:,:2],0) for i in b] if boundingBox else [[] for i in b]
        c3 = [numpy.amin(i[:,:2],0) for i in b] if boundingBox else [[] for i in b]
        c4 = [numpy.mean(i[:,:2],0) for i in b] if centroid else [[] for i in b]
        c = [numpy.concatenate(i) for i in zip(c,c1,c2,c3,c4)]
        out = [numpy.vstack(c)]
        for num,lev in higherSigs:
            sigand = [numpy.concatenate(i) for i in segmentList(sigand,num)]
            c = [sig(i,lev) for i in sigand]
            out.append(numpy.vstack(c))
        return out
    pyramid = True
    postfix = "#".join([("("+str(num)+","+str(lev)+")") for num,lev in higherSigs])
    encodingString = "localSigsPyramid" + ("*" if speedLooksAtFirstTwoDims else "") + str(level)+"_"+str(minilength)+"_"+str(overlap)+"_"+str(seqlength)+("_2d" if sig2dOnly else "")+str("bb" if boundingBox else "")+str("ctr" if centroid else "")+"#"+postfix

def transform_dc(x):
    return transform(manipulateCharacters.distortCharacter(x,0.3,0.3,0.03,True))

def trainingDataAugmented():
    global pool
    trainX,trainY=manipulateCharacters.pickleLoad("1.1.split/train-%d.pickle"%numpy.random.randint(0,100))
    X=[transform_dc(x) for x in trainX]
    if not pyramid:
        maxTime=max([char.shape[0] for char in X])
        X=numpy.vstack([numpy.vstack([
            numpy.zeros((maxTime-x.shape[0],x.shape[1]),dtype='float32'),x])[None,:,:] for x in X])
    else:
        X1=[]
        for i in range(len(X[0])):
            maxTime=max([char[i].shape[0] for char in X])
            if i==0: #more care needed with more than 2 inputs
                while maxTime % higherSigs[0][0] != 0:
                    maxTime = maxTime +1
            X1.append(numpy.vstack([numpy.vstack([
                numpy.zeros((maxTime-x[i].shape[0],x[i].shape[1]),dtype='float32'),x[i]])[None,:,:] for x in X]))
        X=X1
    if oneHotLabels:
        Y=numpy.eye(nClasses)[trainY]
    else:
        Y = numpy.expand_dims(trainY,-1)
    return X, Y
def testData():
    global pool
    testX,testY=manipulateCharacters.pickleLoad("1.1.split/test-%d.pickle"%numpy.random.randint(0,1000))
    X=[transform(x) for x in testX]
    if not pyramid:
        maxTime=max([char.shape[0] for char in X])
        X=numpy.vstack([numpy.vstack([
            numpy.zeros((maxTime-x.shape[0],x.shape[1]),dtype='float32'),x])[None,:,:] for x in X])
    else:
        X1=[]
        for i in range(len(X[0])):
            maxTime=max([char[i].shape[0] for char in X])
            if i==0: #more care needed with more than 2 inputs
                while maxTime % higherSigs[0][0] != 0:
                    maxTime = maxTime +1
            X1.append(numpy.vstack([numpy.vstack([
                numpy.zeros((maxTime-x[i].shape[0],x[i].shape[1]),dtype='float32'),x[i]])[None,:,:] for x in X]))
        X=X1
    if oneHotLabels:
        Y=numpy.eye(nClasses)[testY]
    else:
        Y = numpy.expand_dims(testY,-1)
    return X, Y

def batchBuilderThread(q,i):
    numpy.random.seed(i)
    while True:
        q.put((trainingDataAugmented(),testData()))

nThreads = multiprocessing.cpu_count()
if "JR_DATA_THREADS" in os.environ:
    nThreads=int(os.environ["JR_DATA_THREADS"])

batchQueue=multiprocessing.Queue(20)
threads=[multiprocessing.Process(
        target=batchBuilderThread,
        args=(batchQueue,i))
         for i in range(nThreads)]
for thread in threads:
    thread.daemon = True
    thread.start()  ## Create batches in parallel

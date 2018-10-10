import os
import validateEnv
validateEnv.check("JR",["JR_NHIDDEN","JR_LEVEL","JR_SIGDIM","JR_DATALENGTH","JR_DATAKNOWNLENGTH","JR_DATAEXTRALENGTH", "JR_MIDLEVELWIDTH"])
os.environ["THEANO_FLAGS"]="floatX=float32,device=cpu,optimizer=fast_compile"
#os.environ["THEANO_FLAGS"]="floatX=float32,device=cpu,cxx="
#os.environ["THEANO_FLAGS"]="floatX=float32,device=cpu,mode=DebugMode"
#os.environ["THEANO_FLAGS"]="floatX=float32,device=gpu0,force_device=True,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8"
#os.environ["THEANO_FLAGS"]="floatX=float32,device=gpu0,force_device=True,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8,base_compiledir=/run/user/1001/theano"
#os.environ["THEANO_FLAGS"]="floatX=float32,device=cpu,force_device=True"
#os.environ["THEANO_FLAGS"]="floatX=float32,device=cpu,force_device=True,mode=NanGuardMode,exception_verbosity=high,NanGuardMode.inf_is_error=False,NanGuardMode.big_is_error=False,NanGuardMode.action=warn,optimizer=fast_compile"
os.environ["KERAS_BACKEND"]="theano"

import theano, numpy, sys, json, hashlib, time
 
#add the parent directory, so we find our iisignature build if it was built --inplace
#sys.path.append(os.path.dirname(os.path.abspath(os.path.dirname(__file__))))
import results

from iisignature_lstm_keras import LSTMSig
import keras.models, keras.layers.recurrent
from keras.layers.core import Dense
from keras.layers.recurrent import SimpleRNN, LSTM
from keras.layers.normalization import BatchNormalization
from six.moves import range
from six import print_

length = int(os.getenv("JR_DATALENGTH") or 15)
knownLength = int(os.getenv("JR_DATAKNOWNLENGTH") or 9)
extraPoints=int(os.getenv("JR_DATAEXTRALENGTH") or 0)
def generate():
    out = []
    n = 11000
    for i in range(n):
        a = numpy.random.binomial(1,0.5)
        if a:
            start=numpy.random.binomial(1,0.5,length+extraPoints)
            if extraPoints>0:
                start[-extraPoints:]=0
            out.append((start,1))
        else:
            start=numpy.random.binomial(1,0.5,length+extraPoints)*2-1
            for j in range(knownLength,length):
                start[j] = numpy.prod(start[(j-knownLength):j])
            start = (1+start)//2
            if extraPoints>0:
                start[-extraPoints:]=0
            out.append((start,0))
    x = numpy.vstack((a for (a,b) in out))
    y = numpy.array([b for (a,b) in out])
    x=x.reshape(x.shape+(1,))
    return x,y
            

nn=keras.models.Sequential()
input_length = length+extraPoints
#nn.add(RecurrentSig(30,sig_level=1,input_shape=(input_length,1),return_sequences=False))
#nn.add(RecurrentSig(30,sig_level=1,use_signatures=False,input_shape=(input_length,1),return_sequences=False)
n_hidden=20
sig_dimension=20
level=2
if "JR_NHIDDEN" in os.environ:
    n_hidden=int(os.environ["JR_NHIDDEN"])
if "JR_LEVEL" in os.environ:
    level=int(os.environ["JR_LEVEL"])
if "JR_SIGDIM" in os.environ:
    sig_dimension=int(os.environ["JR_SIGDIM"])
mid_level_width =int(os.getenv("JR_MIDLEVELWIDTH") or 40)
nn.add(LSTMSig(n_hidden,sig_dimension,sig_level=level,input_shape=(input_length,1),return_sequences=False))
#nn.add(LSTM(140,input_shape=(input_length,1),return_sequences=False))
#nn.add(SimpleRNN(80,input_shape=(input_length,1),return_sequences=False))
#nn.add(BatchNormalization())
nn.add(Dense(mid_level_width,activation="relu"))
nn.add(Dense(1,activation="sigmoid"))
clipvalue=5
op = keras.optimizers.Adam(lr=0.0002, beta_1=0.9, beta_2=0.999, epsilon=1e-8, clipvalue=clipvalue)
#nn.compile(loss='mse', optimizer=op,metrics = ["accuracy"])
nn.compile(loss='binary_crossentropy', optimizer=op,metrics = ["accuracy"])
nn.summary()

modelString = nn.to_json(sort_keys=True)
tag=hashlib.md5(modelString.encode("ascii")).hexdigest()[:6]
filename="LSTMSig_isRandom"+tag+str(length)+"."+str(knownLength)+"."+str(extraPoints)+".model"

continuationMsg=""
if False:
    continuationMsg=filename
    nn.load_weights("models/"+filename)

attribs={"sigdim":sig_dimension, "siglevel":level, "type":"LSTMSIG", "n_hidden":n_hidden, "datalength":length, "dataknownlength":knownLength, "dataextrapoints":extraPoints, "midlevelwidth":mid_level_width}
if clipvalue!=0:
    attribs["clip"]=clipvalue
results.startRun(continuationMsg, attribs, modelString, json.dumps(op.get_config(),sort_keys=True), (os.path.abspath(__file__),os.path.abspath("iisignature_lstm_keras.py")))

x,y = generate()
testx = x[-1000:,:,:]
testy = y[-1000:]
x=x[:-1000,:,:]
y=y[:-1000]
#for i,j in enumerate(y):
#    print j, x[i,:,0]
#a=numpy.random.uniform(size=(3,5,3))
print (y.shape)
#print nn.predict(x)
#nn.fit(x,y,nb_epoch=45,shuffle=0,batch_size=134,validation_data=(testx,testy))
testx,testy=generate()
#print (nn.evaluate(testx,testy,verbose=0))
#z=nn.predict(testx[0:20])
#for i in range(len(z)):
#    print (z[i], testy[i], testx[i,:,0], numpy.sum(testx[i,:-1,0]))
doTest=False
for ctr in range(500):
    print (ctr)
    vd = (testx,testy) if doTest else None
    a=nn.fit(x,y,epochs=1,shuffle=0,batch_size=128,validation_data=vd)
    b=a.history
    trainLoss=b["loss"][0]
    results.step(trainLoss, b["acc"][0], b["val_loss"][0] if doTest else 0, b["val_acc"][0] if doTest else 0)
    if numpy.isnan(trainLoss):
        sys.exit()  #it's good to let the db know there was a nan
    if (1+ctr)%100==0:
        nn.save_weights("models/"+filename,overwrite=True)

print("evaluating...")
e=nn.evaluate(testx,testy)
print("")
print_ ("objective: ",e[0],"\n accuracy",e[1])
results.addResultAttribute("testObj",e[0])
results.addResultAttribute("testAcc",e[1])

#sys.exit()
print ("exiting at "+ time.strftime("%Y-%m-%d %H:%M:%S"))



                                                        

#Ben Graham and Jeremy Reizenstein, University of Warwick, GPL v3
print("--------------512 units-----------------------sgd")
import os, sys, numpy, loadData, time, hashlib
import validateEnv
validateEnv.check("JR",["JR_LOADW","JR_NO_SQLITE3","JR_RESULTSFILE","JR_NOSAVE","JR_DATA_THREADS"])
print (loadData.encodingString)
#os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0'
#os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0,dnn.enabled=false,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8'
os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0,dnn.enabled=false,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8,lib.cnmem=1'
os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0,dnn.enabled=true,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8,lib.cnmem=1,optimizer=fast_compile'
os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0,dnn.enabled=True,cxx=g++-4.8,nvcc.flags=-D_FORCE_INLINES,nvcc.compiler_bindir=/usr/bin/g++-4.8,lib.cnmem=1,optimizer_excluding=inplace_elemwise_optimizer'
#os.environ["THEANO_FLAGS"] = 'floatX=float32,device=gpu0,dnn.enabled=false,optimizer=fast_compile'
#os.environ["THEANO_FLAGS"] = 'floatX=float32,device=cpu,dnn.enabled=false,optimizer=fast_compile'
#os.environ["KERAS_BACKEND"]="tensorflow"
import results, json, shapes
from keras_wrappers import Densenet, ForgetAllButLastTimestep
from six.moves import range

import keras, keras.models, keras.layers.core, keras.optimizers, keras.layers.recurrent

((trainX,trainY),(testX,testY))=foo=loadData.batchQueue.get()
pyramid = loadData.pyramid
if not pyramid:
    print(trainX.shape, trainY.shape, testX.shape, testY.shape)
print (shapes.shapes(foo))
doTest=True
layer_width = 1024
nLayers = 1
layer=keras.layers.recurrent.LSTM; layername="LSTM"
#layer=keras.layers.recurrent.GRU; layername="GRU"
#layer=keras.layers.recurrent.SimpleRNN; layername="RNN"
densenet = False
densenetMult = 20

nn=keras.models.Sequential()
if pyramid:
    nn.add(layer(500,input_dim=trainX[0].shape[2],return_sequences=True))
    for iinput in range(1,len(trainX)):
        nn.add(keras.layers.pooling.MaxPooling1D(loadData.higherSigs[0][0]))
        nn2 = keras.models.Sequential()
        nn2.add(keras.layers.Permute((1,2),input_shape=(None,trainX[iinput].shape[2])))#layer does nothing, just to add the dim in
        nn3 = keras.models.Sequential()
        nn3.add(keras.layers.Merge([nn,nn2],mode='concat'))
        nn=nn3
        nn.add(layer(layer_width,return_sequences=True))
    nn.add(layer(layer_width,return_sequences=False))
else:
    if nLayers == 1:
        if densenet:
            nn.add(Densenet(layer(layer_width//densenetMult,return_sequences=False),densenetMult,input_dim=trainX.shape[2]))
        else:
            nn.add(layer(layer_width,input_dim=trainX.shape[2],return_sequences=False))
    else:
        if densenet:
            layerWidthUse = layer_width//densenetMult
            nn.add(Densenet(layer(layerWidthUse,return_sequences=True),densenetMult,input_shape=(None,trainX.shape[2])))
            for i in range(1,nLayers):
                nn.add(Densenet(layer(layerWidthUse,return_sequences=True),densenetMult))
            #nn.add(keras.layers.core.Lambda(lambda x: x[:,-1,:], lambda s:(s[0],s[2])))
            nn.add(ForgetAllButLastTimestep())
        else:
            nn.add(layer(layer_width,input_dim=trainX.shape[2],return_sequences=True))
            for i in range(2,nLayers):
                nn.add(layer(layer_width,return_sequences=True))
            nn.add(layer(layer_width,return_sequences=False))
nn.add(keras.layers.core.Dense(loadData.nClasses))
nn.add(keras.layers.core.Activation('softmax'))
#op = keras.optimizers.SGD(lr=0.1, momentum=0.9, decay=0., nesterov=False)
adamLR = 0.001 if nLayers == 0 else 0.0002
if densenet:
    adamLR = 0.038
#op = keras.optimizers.Adam(lr=0.002, beta_1=0.9, beta_2=0.999, epsilon=1e-8)
op = keras.optimizers.Adam()
loss = ('categorical_crossentropy' if loadData.oneHotLabels else "sparse_categorical_crossentropy")
nn.compile(loss=loss, optimizer=op, metrics=["accuracy"])
nn.summary()

modelString = nn.to_json(sort_keys=True)
tag=hashlib.md5(modelString.encode("ascii")).hexdigest()[:6]
filename = "casia11."+layername+"_"+tag+"_."+loadData.encodingString+".model"
loadWeights = ("JR_LOADW" in os.environ)
noSave = ("JR_NOSAVE" in os.environ)

continuationMsg=""
if loadWeights:
    continuationMsg=filename
    nn.load_weights("models/"+filename)

repLength = (trainX[0].shape[2] if pyramid else trainX.shape[2])
results.startRun(loadData.encodingString,repLength,continuationMsg,trainY.shape[0],testY.shape[0],layername+"-allpython",nLayers, layer_width, modelString,json.dumps(op.get_config(),sort_keys=True),(os.path.abspath(__file__),os.path.abspath("loadData.py")))

for ctr in range(0,10**10):
    print (ctr)
    a=nn.fit(trainX, trainY, batch_size=64, nb_epoch=1, verbose=0, validation_data=(testX,testY),shuffle=False)
    b=a.history
    trainLoss = b["loss"][0]
    results.step(trainLoss, b["acc"][0], b["val_loss"][0] if doTest else 0, b["val_acc"][0] if doTest else 0)
    if numpy.isnan(trainLoss):
        sys.exit()  #it's good to let the db know there was a nan
    if ctr%10==0 and not noSave:
        nn.save_weights("models/"+filename,overwrite=True)
    if True and ctr>10000:
        break
    ((trainX,trainY),(testX,testY))=loadData.batchQueue.get()

#sys.exit()
print ("exiting at "+ time.strftime("%Y-%m-%d %H:%M:%S"))
    

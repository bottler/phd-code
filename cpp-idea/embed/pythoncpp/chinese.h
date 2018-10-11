#ifndef JR_CHINESE_H
#define JR_CHINESE_H

//NB: clang doesn't mind extern "C" after the visibility thing, GCC does
#ifdef LINUX
#define LUAFUNC extern "C" __attribute__ ((visibility ("default")))
#else
#define LUAFUNC extern "C"
#endif


LUAFUNC int getStrokeRepnLength();
LUAFUNC void waitForOpenDataCopy();
LUAFUNC void setDataCopyAsync(const char* data);
std::string getStrokeRepnName();

const int RNN_maxTime = 16;
const int nTrain_batch_RNN = 1280;
const int nTest_batch_RNN = 100;
const int nBatches_RNN = 10;

struct RNNInput{
  vector<vector<float>> testX, trainX;
  vector<int> trainChar, testChar; //unused unless REMEMBER_WHICH_CHAR defd
  vector<int> trainY, testY;
};
RNNInput getRNNinput_readyForPython();

RNNInput getRNNinput_variableLength_readyForPython();
LUAFUNC int getVariableLengthStrokeRepnLength();
std::string getVariableLengthRepnName();


////////////

//multilayered
LUAFUNC int numLayers();
LUAFUNC int layerMinibatchSize();
//0-indexed layer numbers
LUAFUNC int layerRepnLength(int layer);
LUAFUNC int layerTimeLength(int layer);
LUAFUNC int layerTimePoolingAfter(int layer);
//dest must be minibatchSize * timeLength * layerRepnLength

struct LayerInput{
  //batch * layer * (time*repnlength)
  vector<vector<vector<float>>> testX, trainX;
  vector<int> trainY, testY;
};
LayerInput getLayerInput_readyForPython();
//LUAFUNC void getLayerData(int layer, float* dest);



#endif

import keras.layers.recurrent
from keras.engine import InputSpec
from keras import backend as K
from keras import initializers, activations
import iisignature
from iisignature_theano import SigJoin, SigScale
import math

#Consider initializing so that state is constant initially or something.
#-perhaps a highway
#add dropout, regularizers

#If sig_level is 1, we can easily avoid using the iisignature library,
#which could be faster.
shortcut_level_1 = True

class LSTMSig(keras.layers.recurrent.Recurrent):
    '''
    A recurrent layer like keras's SimpleRNN or LSTM, which has a signature cell - explained elsewhere

    sig_level: the depth of the signature

    init: the initialisation of the map from input to state

    inner_init: the initialisation of the matrix from state to state

    activation: the activation applied to the output values of the state.
    '''
    def __init__(self, n_hidden, sig_dimension,
                 sig_level=2,
                 kernel_initializer='glorot_uniform',
                 #inner_init='he_normal',#could be good to use 'orthogonal' - not used
                 activation='tanh',#not applied to signature elements
                 **kwargs):
        self.n_hidden = n_hidden
        self.units=n_hidden
        self.sig_dimension = sig_dimension
        self.sig_level = sig_level
        self.sigsize = iisignature.siglength(sig_dimension,sig_level)
        self.kernel_initializer = initializers.get(kernel_initializer)
        self.activation = activations.get(activation)
        self.states=[None,None]
        super(LSTMSig, self).__init__(**kwargs)
        
    def build(self, input_shape):
        self.input_spec = [InputSpec(shape=input_shape)]
        self.input_dim = input_shape[2]
        self.Wf = self.add_weight((self.input_dim+self.n_hidden, self.sig_dimension),
                                  initializer=self.kernel_initializer,
                                  name='{}_Wf'.format(self.name))
        self.bf = self.add_weight(self.sig_dimension,
                                  initializer='zero',
                                  name='{}_bf'.format(self.name))
        self.WC = self.add_weight((self.input_dim+self.n_hidden, self.sig_dimension),
                                  initializer=self.kernel_initializer,
                                  name='{}_WC'.format(self.name))
        self.bC = self.add_weight(self.sig_dimension,
                                  initializer='zero',
                                  name='{}_bC'.format(self.name))
        self.Wi = self.add_weight((self.input_dim+self.n_hidden, self.sig_dimension),
                                  initializer=self.kernel_initializer,
                                  name='{}_Wi'.format(self.name))
        self.bi = self.add_weight(self.sig_dimension,
                                  initializer='zero',
                                  name='{}_bi'.format(self.name))
        self.Wo = self.add_weight((self.input_dim+self.n_hidden, self.n_hidden),
                                  initializer=self.kernel_initializer,
                                  name='{}_Wo'.format(self.name))
        self.bo = self.add_weight(self.n_hidden,
                                  initializer='zero',
                                  name='{}_bo'.format(self.name))

        #check I interpret this right
        self.Wout = self.add_weight((self.sigsize, self.n_hidden),
                                    initializer=self.kernel_initializer,
                                    name='{}_Wout'.format(self.name))
        self.bout = self.add_weight(self.n_hidden,
                                    initializer='zero',
                                    name='{}_bout'.format(self.name))

    def step(self, x, states):
        inp = K.concatenate([x,states[1]],1)
        forget_scales = K.sigmoid(K.dot(inp,self.Wf)+self.bf)
        if shortcut_level_1 and self.sig_level == 1:
            Cdash=states[0] * forget_scales
        else:
            Cdash=SigScale(states[0],forget_scales,self.sig_level)
        i=K.sigmoid(K.dot(inp,self.Wi)+self.bi)
        xtilde=self.activation(K.dot(inp,self.WC)+self.bC)
        i_times_xtilde = i*xtilde # this is equivalent of the document, right?
        if shortcut_level_1 and self.sig_level == 1:
            C=Cdash + i_times_xtilde
        else:
            C=SigJoin(Cdash,i_times_xtilde,self.sig_level)
        o=K.sigmoid(K.dot(inp,self.Wo)+self.bo)
        oo=self.activation(K.dot(C,self.Wout)+self.bout)
        h=o*oo
        if 0:
            #enable this to avoid detached gradient errors when commenting out parts of step()
            useless=K.max(self.bo)+K.max(self.Wo)+K.max(self.bi)+K.max(self.Wi)+K.max(self.bC)+K.max(self.WC)
            useless=useless+K.max(self.bf)+K.max(self.Wf)+K.max(self.bout)+K.max(self.Wout)
            h=h+useless*0
        return h,[C,h]

    def get_initial_states(self, x):
        # x has shape (samples, timesteps, input_dim)
        # build all-zero tensors of shape (samples, whatever)
        initial_state = K.zeros_like(x)  
        initial_state = K.sum(initial_state, axis=(1, 2))  # (samples,)
        initial_state = K.expand_dims(initial_state)  # (samples, 1)
        lengths = (self.sigsize,self.n_hidden)
        initial_states = [K.tile(initial_state, [1, i]) for i in lengths]  # (samples, i)
        return initial_states
    
    def get_config(self):
        config = {'sigsize': self.sigsize,
                  'sig_level': self.sig_level,
                  'sig_dimension': self.sig_dimension,
                  'n_hidden': self.n_hidden,
                  'units': self.units,
                  'kernel_initializer': initializers.serialize(self.kernel_initializer),
                  'activation': self.activation.__name__}
        base_config = super(LSTMSig, self).get_config()
        return dict(list(base_config.items()) + list(config.items()))
    

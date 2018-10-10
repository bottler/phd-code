import free_lie_algebra as fla
import itertools, functools, sys
import numpy as np
from sympy.utilities.iterables import partitions

#sys.path.append("/home/jeremyr/iisignature")

np.set_printoptions(linewidth=100,precision=3,suppress=1)

#a demonstration of two paths whose signatures differ
#but whose FKK signatures match
#The signatures differ at level 5, but the FKK agree up to level 10 at least

def fkk_invariants(letters, depth):
    allInternals=[]
    if depth==1:
        return list(letters)
    for internals in itertools.combinations_with_replacement(letters,depth-1):
        #print ([i.pretty() for i in internals])
        elt = functools.reduce(fla.shuffleProduct,internals)
        allInternals.append(elt)
    out=[]
    for final in letters:
        for internals in allInternals:
            out.append(fla.concatenationProduct(internals,final))
    return out

level=10 #USER
d = 2 #USER

letters_=tuple(range(1,d+1))
letters = tuple(fla.letter2Elt(i) for i in letters_)

#path = np.random.rand((20,d))
#path1=[[0,0],[0,1],[1,1],[1,0],[0,0]]
#path2=[[0,1],[1,1],[1,0],[0,0],[0,1]]
#path2=[[1,1],[1,0],[0,0],[0,1],[1,1]]


#Path1 is a figure 8
#
#               h    bg    c
#
#                    afk
#
#               i    ej    d
path1=[[0,0],[0,1],[2,1],[2,-1],[0,-1],[0,1],[-2,1],[-2,-1],[0,-1],[0,0]]
path2=[[-i,j] for i,j in path1]
path3 = path1 + list(reversed(path2))

fkks = [i for l in range(1,level+1) for i in fkk_invariants(letters, l)]

sig = fla.signature_of_path_manual
#sig = fla.signature_of_path_iisignature
sig1 = sig(path1,level)
sig2 = sig(path2,level)
sig3 = sig(path3,level)
fkk1 = np.array([fla.dotprod(sig1,i) for i in fkks])
fkk2 = np.array([fla.dotprod(sig2,i) for i in fkks])
fkk3 = np.array([fla.dotprod(sig3,i) for i in fkks])

twopaths=False
if twopaths:
    print((sig1-sig2).pretty(tol=1e-14))
    #print(sig1)
    #print(sig1.pretty())#
    #print(sig2.pretty())
    #print(fkk1)
    #print(fkk2)
    print(fkk2-fkk1)
    #print (np.column_stack([fkk1,fkk2,fkk2-fkk1]))
    
if not twopaths:
    print(fkk3)
    import matplotlib.pyplot as plt

    def xdy(x,y):
        result = 0.
        for i in range(len(x)-1):
            result += 0.5 * ( x[i+1] + x[i] - 2 * x[0] ) * (y[i+1] - y[i])
        return result

    def test_xdy():
        path = np.random.uniform( size=(20,2) )
        print(path)
        s = sig(path, 2)
        #a=fla.Word("12")
        a=fla.word2Elt("12")
        assert( fla.dotprod(s,a) == xdy( path[:,0], path[:,1] ) )
        #signature = iisignature . sig ( path ,4)
        #print(signature)

    x1 = np.array( list ( map( lambda xy: xy[0], path1 ) ) )
    y1 = np.array( list ( map( lambda xy: xy[1], path1 ) ) )
    x3 = np.array( list ( map( lambda xy: xy[0], path3 ) ) )
    y3 = np.array( list ( map( lambda xy: xy[1], path3 ) ) )
    #plt.plot( range(len(x3)), x3 )
    #plt.plot( range(len(x3)), y3 )
    #plt.show()

    for n in range(30):
        for m in range(30):
            assert( xdy( x3**n * y3**m, x3) == 0 )
            assert( xdy( x3**n * y3**m, y3) == 0 )
            #print( xdy( x3**n * y3**m, y3))



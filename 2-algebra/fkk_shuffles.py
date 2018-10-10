from free_lie_algebra import *
import free_lie_algebra
import itertools, functools, sys
import numpy as np
from sympy.utilities.iterables import partitions

np.set_printoptions(linewidth=100)

#Show that the shuffle closure of the FKK "signature" elements
#is not the whole of tensor space

def fkk_invariants(letters, depth):
    allInternals=[]
    if depth==1:
        return list(letters)
    for internals in itertools.combinations_with_replacement(letters,depth-1):
        #print ([i.pretty() for i in internals])
        elt = functools.reduce(shuffleProduct,internals)
        allInternals.append(elt)
    out=[]
    for final in letters:
        for internals in allInternals:
            out.append(concatenationProduct(internals,final))
    return out


level=4 #USER
d = 3 #USER

letters_=tuple(range(1,d+1))
letters = tuple(letter2Elt(i) for i in letters_)

b=TensorSpaceBasis(word2Elt,None,d,level)

if 1:
    fkk = fkk_invariants(letters, level)
    for i in fkk:
        print(i.pretty())
    print("fkk spans {} out of the {} dimensions".format(b.rank(fkk),d**level))
    print(countHallWords(d,level))

if 1:
    def shuffleList(a):
        return functools.reduce(shuffleProduct,a)

    fkks=[fkk_invariants(letters,i) for i in range(1,level+1)]
    allFkkShuffles=expandThroughGrading(fkks,level,shuffleList)
    rank=b.rank(allFkkShuffles)
    print("fkk shuffles span {} out of the {} dimensions".format(rank,d**level))

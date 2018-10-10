#vaguely useful for understanding the shape of an unknown large object which may be a load of nested lists and tuples.
def shapes(a):
    if type(a)==list or type(a) == tuple:
        return [shapes(e) for e in a]
    if type(a) in (int,float):
        return 0
    return a.shape

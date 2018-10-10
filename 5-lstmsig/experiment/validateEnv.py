import os

#throw error if an env var name beginning with prefix is not in the list variables
#we take whole variables so that they are easy to grep.
def check(prefix, variables):
    for i in os.environ.keys():
        if i.startswith(prefix):
            if i not in variables:
                #no need - see suggestions in the call stack anyway
                #raise ValueError("unknown env var", i, " perhaps you meant ",variables)
                raise ValueError("unknown env var", i)

This is not code used in any of the final experiments. The idea here was to do chinese handwriting experiments where the whole of the opening the data files, picking training batches, and representation of the characters was in C++. This code would generate the necessary Python or lua addin. This work was done around 2014-5. Some of the representation code here I know is broken.

There's some other stuff here around making some of the functionality available to lua, reporting results in the style of results.py when your python has no sqlite3 module, etc.

Signature.h contains a template-metaprogramming type signature calculation, which may be interesting.
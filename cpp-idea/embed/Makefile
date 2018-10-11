#Try using ramdisk?

#C=clang++
HOST=$(shell hostname)
C=g++-4.7 -Wno-tautological-compare #good on cow
C_=gcc-4.7
PYTHON=python2.6
ifeq ($(HOST),fifi)
C=g++-4.8 -Wno-unused-local-typedefs #good on jfr.jumpingcrab.com
C_=gcc-4.8
C=clang++-3.6 -Wno-tautological-compare
PYTHON=python2.7
endif

SHARED=../Shared
PCH=pythoncpp/stdafx.h.gch

ifeq ($(HOST),fifi)
LIBS=-L/home/jeremyr/libs/boost_1_59_0/stage/lib -lpython2.7 -lboost_python -lboost_system -lpthread -ldl
#INC=-I/home/jeremyr/libs/boost_1_59_0 -I/usr/include/python2.7 -I/usr/lib64/python2.6/site-packages/numpy/core/include -I/home/jeremyr/libs/eigen-eigen-b30b87236a1b -I$(SHARED)
INC=-I/home/jeremyr/libs/boost_1_59_0 -I/usr/include/python2.7 -I/home/jeremyr/libs/eigen-eigen-b30b87236a1b -I$(SHARED)
#Change to python 3.5
LIBS3=-L/usr/lib/python3.5/config-3.5m-x86_64-linux-gnu -lpython3.5
INC3=-I/usr/include/python3.5 -I/home/jeremyr/libs/eigen-eigen-b30b87236a1b -I$(SHARED)
else
INC=-I/usr/include/python2.6 -I/usr/lib64/python2.6/site-packages/numpy/core/include -I../../../libs/eigen-eigen-10219c95fe65 -I$(SHARED)
LIBS=-lpython2.6 -lboost_python
endif
FLAGS=-Werror 


ifneq "undefined" "$(origin JR_TINIS)"
C=g++
C_=gcc
#LIBS=-ldl -lpthread -lboost_system -lboost_python -lpython2.7
LIBS=-lpython2.7
FLAGS= 
PYTHON=python2.7
INC=-I/home/maths/phrnai/libs/eigen-eigen-b30b87236a1b -I../Shared -I/csc/tinis/software/MPI/GCC/4.9.3-binutils-2.25/OpenMPI/1.10.2/Python/2.7.11/lib/python2.7/site-packages/numpy/core/include/ -I/csc/tinis/software/MPI/GCC/4.9.3-binutils-2.25/OpenMPI/1.10.2/Python/2.7.11/include/python2.7/
endif

O:=Representations.o RotationalInvariants.o RotationalInvariants2.o pythoncpp.o handwritingData.o 
O2:=chinese.o bchplay.o utils.o #these ones are those that depend on bch.h 

#use gnu++11

FLAGS:=$(FLAGS) -pipe -fno-common -std=c++11 -march=native -DLINUX -Wall -Wno-missing-braces -Winvalid-pch -Wno-unused-function -Wno-unused-variable -Wno-sign-compare -fvisibility=hidden -fvisibility-inlines-hidden #-O #-Wfatal-errors #decide your optimisation level here

ifeq ($(findstring clang++,$(C)),clang++)
	FLAGS:=$(FLAGS) -Wno-unused-local-typedefs
else
        FLAGS:=$(FLAGS) -Wno-maybe-uninitialized -Wno-unused-but-set-variable
endif

build: embed.so try

.PHONY: clean rebuild newpch try build h b testsig

rebuild: clean build

newpch: #if you have changed some header, you can use "make newpch build" instead of "rebuild" to avoid rebuilding bch.o .
	rm -f $(PCH)

clean: 
	rm -f embed.so baresig.so baresig3.so report.so *.o $(PCH)

sql.o: 
	$(C_) -c $(SHARED)/sqlite/sqlite3.c -fvisibility=hidden -fPIC -o $@

jrsqlite.o: $(SHARED)/jrsqlite/jrsqlite.cpp
	$(C) -c $(INC) $(FLAGS) $^ -fPIC -o $@

$(PCH): pythoncpp/stdafx.h
	nice -10 $(C) $(INC) $(FLAGS) -fPIC -o $@ -x 'c++-header' $<

$(O) : %.o : pythoncpp/%.cpp $(PCH) # can use $$(patsubst) with .SECONDEXPANSION
	nice -10 $(C) -c $(INC) $(FLAGS) $< -fPIC -o $@

$(O2) : %.o : pythoncpp/%.cpp pythoncpp/bch.h $(PCH) #dependency on bch.h
	nice -10 $(C) -c $(INC) $(FLAGS) $< -fPIC -o $@

bch.o : pythoncpp/bch.cpp pythoncpp/bch.h #special as does not depend on PCH, and build takes ages
	nice -10 $(C) -c $(INC) $(FLAGS) $< -fPIC -o $@ 

#put bch.o first in this dependency list so that under e.g. 'make -j 2' it happens early
embed.so: bch.o $(O) jrsqlite.o sql.o $(O2)
	$(C) $(FLAGS) $^ $(LIBS) -shared -fPIC -o embed.so #-Wl,--unresolved-symbols=ignore-in-shared-libs #or report-all

report.o: reporter/report.cpp #$(PCH)
	nice -10 $(C) -c $(INC) $(FLAGS) $< -fPIC -o $@

report.so: jrsqlite.o sql.o report.o
	$(C) $(FLAGS) $^ $(LIBS) -shared -fPIC -o $@ #-Wl,--unresolved-symbols=ignore-in-shared-libs #or report-all

baresig.so: BareSigAddin/baresig.cpp
	nice -10 $(C) $(INC) $(FLAGS) $< -fPIC -shared $(LIBS) -o $@
baresig3.so: BareSigAddin/baresig.cpp
	nice -10 $(C) $(INC3) $(FLAGS) $< -fPIC -shared $(LIBS3) -o $@

#small executable when you just need an executable, 
#doesn't use embed.so, but depends on it so that it builds last.
a : pythoncpp/chinese.cpp pythoncpp/executable_a.cpp embed.so
	$(C) $(INC) $(FLAGS)  pythoncpp/executable_a.cpp sql.o chinese.o jrsqlite.o handwritingData.o Representations.o $(LIBS) -fPIC -o a

##
try: embed.so #this detects unresolved symbols, but doesn't undecorate them
	$(PYTHON) -c "import embed"
h: embed.so
	$(PYTHON) -c "import embed; help(embed)"
b: build
	$(PYTHON) -c "import embed; embed.bch()"
testsig: embed.so
	#$(PYTHON) -c "import embed, numpy; a=numpy.array([[3,4],[3,2],[3,14]]).astype('float32'); print embed.sig(a); print embed.sigs([a,a])"
	#$(PYTHON) -c "import embed, numpy; a=numpy.array([[3,4,2],[3,2,2],[1,3,14]]).astype('float32'); print embed.sigs([a],4); print embed.sigsRescaled([a],4)"
	$(PYTHON) -c "import embed, numpy; numpy.set_printoptions(suppress=True); a=numpy.array([[3,4],[1,2],[3,14]]).astype('float32'); print embed.sigs([a],4); print embed.sigsRescaled([a],4); print embed.sigsWithoutFirsts([a],4)"
	$(PYTHON) -c "import embed, numpy; numpy.set_printoptions(suppress=True); a=numpy.array([[3,4],[1,2],[3,14]]).astype('float32'); print embed.logsigs([a],4); print embed.logsigsRescaled([a],4)"

#Third party libraries:
#numpy_boost
#boost
#numpy
#libalgebra

#TODO
#understand why -fvisibility-hidden is failing - only the python things need exporting
#fvisibility-hidden works to a certain extent
#(see exports with nm -D foo.o)
#use -O3 or at least something!

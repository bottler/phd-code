#ifndef JR_PYTHONUTILS_H
#define JR_PYTHONUTILS_H

//THIS IS A GROUP OF FUNCTIONS DESIGNED JUST TO ENABLE
// PLAYING WITH SIGNATURES IN PYTHON
//Most of these are only for 2d input

numpy_boost<float,1> sig_(numpy_boost<float,2> path, int level);
boost::python::list sigs_(boost::python::list paths, int level);
boost::python::list sigsWithoutFirsts_(boost::python::list paths, int level);
boost::python::list sigsRescaled_(boost::python::list paths, int level);

numpy_boost<float,1> sigupto_(numpy_boost<float,2> path, int level, int infolevel);
boost::python::list sigsupto_(boost::python::list paths, int level, int infolevel);

numpy_boost<float,1> logsig_(numpy_boost<float,2> path, int level);
boost::python::list logsigs_(boost::python::list paths, int level);
boost::python::list logsigsRescaled_(boost::python::list paths, int level);

//The following functions are not restricted to 2d
numpy_boost<float,1> sig_notemplates_(numpy_boost<float,2> path, int level);
numpy_boost<float,1> sig_general_(numpy_boost<float,2> path, int level);
numpy_boost<float,1> logsig_general_(numpy_boost<float,2> path, int level);
numpy_boost<float,1> logsigfromsig_(numpy_boost<float,2> path, int level);

#endif

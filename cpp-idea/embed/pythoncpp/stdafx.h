// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#ifndef STDAFX_PYTHONCPP_HANDWRITING

#ifndef LINUX

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#undef min
#undef max
#endif

#include <array>
#include <boost/python/module.hpp>
#include <boost/python/def.hpp>
#undef min
#undef max
#include <string>
#include <stdexcept>
#include <iostream>
#include <random>
#include <utility>
#include <bitset>
#include <iomanip>
#include <set>
#include <future>

#include "jrsqlite/JRSqlite.h"


#include "libalgebra/libalgebra.h"
#include "jrutils/randomutils.h"

#include "Signature.h"
#include "RotationalInvariants.h"

#ifndef LINUX
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy_boost.hpp"
#include "numpy_boost_python.hpp"
//#include "Eigen/Eigen"
#endif 

using std::cout;
using std::endl;
using std::pair;
using std::string;
using std::vector;


#endif

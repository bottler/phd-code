#include<utility>
#include<list>
#include<vector>
#include<utility>
#include<memory>
#include"../../Shared/jrsqlite/JRSqlite.h"
#include<Python.h>

//This is a python addin to write results (etc) to sqlite which doesn't use boost python.
//It should be buildable on tinis.
//no transactions here - every stmt is its own transaction

#define BE_OPEN if(!g_conn){PyErr_SetString(PyExc_RuntimeError,"db not opened"); return NULL;}

std::unique_ptr<JRSqlite::Connection> g_conn;

void open(const char* location){
  g_conn.reset(new JRSqlite::Connection (location));
}

static PyObject *
open_(PyObject *self, PyObject *args){
  const char* location;
  //  PyObject* a1, a2;
  //if (!PyArg_ParseTuple(args, "OO", &a1, &a2));
  if (!PyArg_ParseTuple(args, "s", &location))
    return NULL;
  open(location);
  Py_RETURN_NONE;
}

void resultless(const char* sql){
  JRSqlite::SimpleStmt s(*g_conn, sql);
  s.execute();
}
static PyObject *
resultless_(PyObject *self, PyObject *args){
  BE_OPEN;
  const char* sql;
  if (!PyArg_ParseTuple(args, "s", &sql))
    return NULL;
  try{
    resultless(sql);
  } catch(std::exception& e){
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}

int lastRow(){
  return (int) g_conn->lastInsertRowid();
}

static PyObject *
lastRow_(PyObject *self, PyObject *args){
  BE_OPEN;
  if (!PyArg_ParseTuple(args, ""))
    return NULL;
  return PyInt_FromLong(lastRow());
}

//runs the statement once, binding the values in its tuple argument
static PyObject *
sink(PyObject *self, PyObject *args){
  BE_OPEN;
  const char* sql; PyObject* t;
  using JRSqlite::SinkAnythingStmt;
  using D = SinkAnythingStmt::D;
  if(!PyArg_ParseTuple(args, "sO", &sql, &t))
    return NULL;
  if(!PyTuple_Check(t) && Py_None != t){
    PyErr_SetString(PyExc_RuntimeError, "second arg must be a tuple or None");
    return NULL;
  }
  std::vector<D> d;
  std::list<std::string> tempHoldingArea;
  if(PyTuple_Check(t)){
    const size_t size = PyTuple_Size(t);
    for(int i=0; i<size; ++i){
      D a;
      PyObject* pyo = PyTuple_GetItem(t,i);
      if(PyFloat_Check(pyo)){
	a.m_type = D::D_;
	a.m_d = PyFloat_AsDouble(pyo);
      } else if (PyLong_Check(pyo)){
	a.m_type = D::I_;
	a.m_i = PyLong_AsLong(pyo);
      } else if (PyInt_Check(pyo)){
	a.m_type = D::I_;
	a.m_i = PyInt_AsLong(pyo);
      } else if (PyString_Check(pyo)){
	a.m_type = D::S_;
	tempHoldingArea.push_back(PyString_AsString(pyo));
	a.m_s = tempHoldingArea.back().data();
      } else if (PyUnicode_Check(pyo)){
	a.m_type = D::S_;
	PyObject* utf8 = PyUnicode_AsUTF8String(pyo); //hopefully this is good enough
	tempHoldingArea.push_back(PyString_AsString(utf8));
	Py_DECREF(utf8);
	a.m_s = tempHoldingArea.back().data();
      } else {
	PyErr_SetString(PyExc_RuntimeError, "second arg was tuple containing something other than string, float, long");
	return NULL;
      }
      d.push_back(a);
    }
  }
  try{
    SinkAnythingStmt s(*g_conn, sql);
    s.execute(d);
  } catch(std::exception& e){
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  Py_RETURN_NONE;
}

//runs the statement once, binding the values in its tuple argument
//return tuple
static PyObject *
sinksource(PyObject *self, PyObject *args){
  BE_OPEN;
  const char* sql; PyObject* t; const char* outTypes;
  using JRSqlite::SinkSourceAnythingStmt;
  using D = SinkSourceAnythingStmt::D;
  if(!PyArg_ParseTuple(args, "sOs", &sql, &t, &outTypes))
    return NULL;
  if(!PyTuple_Check(t) && Py_None != t){
    PyErr_SetString(PyExc_RuntimeError, "second arg must be a tuple or None");
    return NULL;
  }
  std::vector<D> in;
  std::list<std::string> tempHoldingArea;
  if(PyTuple_Check(t)){
    const size_t size = PyTuple_Size(t);
    for(int i=0; i<size; ++i){
      D a;
      PyObject* pyo = PyTuple_GetItem(t,i);
      if(PyFloat_Check(pyo)){
	a.m_type = D::D_;
	a.m_d = PyFloat_AsDouble(pyo);
      } else if (PyLong_Check(pyo)){
	a.m_type = D::I_;
	a.m_i = PyLong_AsLong(pyo);
      } else if (PyInt_Check(pyo)){
	a.m_type = D::I_;
	a.m_i = PyInt_AsLong(pyo);
      } else if (PyString_Check(pyo)){
	a.m_type = D::S_;
	tempHoldingArea.push_back(PyString_AsString(pyo));
	a.m_s = tempHoldingArea.back().data();
      } else if (PyUnicode_Check(pyo)){
	a.m_type = D::S_;
	PyObject* utf8 = PyUnicode_AsUTF8String(pyo); //hopefully this is good enough
	tempHoldingArea.push_back(PyString_AsString(utf8));
	Py_DECREF(utf8);
	a.m_s = tempHoldingArea.back().data();
      } else {
	PyErr_SetString(PyExc_RuntimeError, "second arg was tuple containing something other than string, float, long");
	return NULL;
      }
      in.push_back(a);
    }
  }
  std::vector<D::Type> outTypesV;
  std::string outTypes_ (outTypes);
  for(char c : outTypes_){
    switch(c){
    case 's':
      outTypesV.push_back(D::S_);
      break;
    case 'i':
      outTypesV.push_back(D::I_);
      break;
    case 'd':
    case 'f':
      outTypesV.push_back(D::D_);
      break;
    default:
      PyErr_SetString(PyExc_RuntimeError, "third arg was tuple containing something other than s (for string), f (for float) or i (for number)");
      return NULL;
    }
  }
  std::list<std::string> temp;
  std::vector<std::vector<D>> out;
  try{
    SinkSourceAnythingStmt s(*g_conn, sql);
    s.execute(in,outTypesV,out,temp);
  } catch(std::exception& e){
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  } 
  PyObject* o = PyTuple_New(out.size());
  for(size_t i=0; i<out.size(); ++i){
    const std::vector<D>& d = out[i];
    PyObject* oo = PyTuple_New(d.size());
    for(size_t j=0; j<d.size(); ++j){
      const D& dd = d[j];
      PyObject* p;
      switch (dd.m_type){
      case D::D_:
	p = PyFloat_FromDouble(dd.m_d);
	break;
      case D::I_:
	p = PyInt_FromLong(dd.m_i);
	break;
      case D::S_:
	p = PyString_FromString(dd.m_s);
	break;
      }
      PyTuple_SET_ITEM(oo,j,p);
    }
    PyTuple_SET_ITEM(o,i,oo);
  }
  return o;
}

static PyMethodDef SpamMethods[] = {
    {"open",  open_, METH_VARARGS, "open(\"foo.sqlite\") opens the connection"},
    {"resultless",  resultless_, METH_VARARGS, "resultless(\"create table A(B, C)\") runs the statement in a transaction"},
    {"lastRow",  lastRow_, METH_VARARGS, "lastRow() returns sqlite's lastrowid - useful for keeping track"},
    {"sink", sink, METH_VARARGS, "sink(\"insert into A values (?,?)\",(28,\"hello\")) runs the statement once in a transaction. The bound values are given as a tuple of strings, doubles or integers."},
    {"sinksource", sinksource, METH_VARARGS, "sinksource(\"select y,z from A where x=?\",(28,),\"si\") runs the statement once in a transaction. The bound values are given as a tuple of strings, doubles or integers. The returned column types here would be string and integer"},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

extern "C" 
__attribute__ ((visibility ("default")))
void
//PyMODINIT_FUNC
initreport(void)
{
    (void) Py_InitModule("report", SpamMethods);
}

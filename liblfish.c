/*
 * Copyright(c) 2014 - Krister Hedfors
 *
 * BEWARE! this code contains severe memory leaks. Each call to
 * a hooked function will leak some data. There are probably
 * other bugs as well in the management of python objects.
 *
 * This code is therefore not suitable for long running programs,
 * but MAY work just fine for commands such as netstat or ps.
 *
 * TODO:
 *  handle exceptions from Hooks
 *
 */
#define _GNU_SOURCE
//#define Py_DEBUG
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <python2.7/Python.h>

#define DEBUG
#include "debug.h"


typedef int (*orig_open_f_type)(const char*, int, ...);
typedef FILE  *(*orig_fopen_f_type)(const char*, const char*);

orig_open_f_type orig_open;
orig_fopen_f_type orig_fopen;


PyObject *eval(const char*);


int _initialized = 0;
PyObject *hooks = NULL;

void init(void)
{
  if (_initialized)
    return;
  debug_fp = stderr;

  orig_open  = (orig_open_f_type)  dlsym(RTLD_NEXT, "open");
  orig_fopen = (orig_fopen_f_type) dlsym(RTLD_NEXT, "fopen");

  //debug_fp = orig_fopen("/tmp/liblfish.log", "a");

  Py_Initialize();
  PyRun_SimpleString("__import__('sys').path.append('" LFISH_ROOT "')");
  hooks = eval("__import__('lfish').Hooks()");
  Py_INCREF(hooks);


  _initialized = 1;
}


PyObject *eval(const char *str)
{
  PyObject *o;
  PyObject *globals = PyDict_New();
  PyObject *locals = PyDict_New();

  PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());

  if ((o = PyRun_String(str, Py_eval_input, globals, locals)) == NULL)
  {
    PyErr_Print();
    exit(1);
  }
  //Py_XDECREF(globals);
  //Py_XDECREF(locals);

  return o;
}


long xlong(PyObject *o)
{
  long ret;
  if ((ret = PyInt_AsLong(o)) == -1)
  {
    PyErr_Print();
    exit(1);
  }
  return ret;
}


char *repr(PyObject *o)
{
  char *s;
  _ENTER0(repr)

  if (o == NULL)
    return "NULL";

  if ((o = PyObject_Repr(o)) == NULL)
  {
    PyErr_Print();
    exit(1);
  }

  if ((s = PyString_AsString(o)) == NULL)
    PyErr_Print(), exit(1);

  if (s[strlen(s) - 1] == '\n')
    s[strlen(s) - 1] = '\0';

  _LEAVE
  return strdup(s);
}


PyObject *pycall_args(PyObject *callable, const char *format, ...)
{
  va_list va;
  PyObject *ret;
  PyObject *args;
  char *sargs;
  _ENTER0(pycall_args)

  va_start(va, format);
  args = Py_VaBuildValue(format, va);
  Py_INCREF(args);
  va_end(va);

  debug("args = 0x%08x", (unsigned int)args);
  sargs = repr(args);
  debug("repr(args) = %s", sargs);

  ret = PyObject_Call(callable, args, NULL);
  //Py_INCREF(ret); //HM
  Py_XDECREF(args);

  _LEAVE
  return ret;
}


PyObject *_pycall_args(PyObject *callable, const char *format, const char *arg)
{
  PyObject *ret;
  PyObject *args;
  char *sargs;
  _ENTER0(pycall_args)


  // nagot aer fel med args, oklart vad
  debug("ARG IS: \"%s\"", arg);
  args = Py_BuildValue(format, arg);
  Py_INCREF(args);

  debug("args = 0x%08x", (unsigned int)args);
  sargs = repr(args);
  debug("repr(args) = %s", sargs);

  ret = PyObject_Call(callable, args, NULL);
  //Py_INCREF(ret); //HM
  Py_XDECREF(args);

  _LEAVE
  return ret;
}


int do_hook_open_pathname(const char *pathname)
{
  int ret;
  PyObject *o;
  PyObject *filt;

  _ENTER0(do_hook_open_pathname)

  filt = PyObject_GetAttrString(hooks, "do_hook_open_pathname");
  Py_INCREF(filt);
  o = pycall_args(filt, "(s)", pathname);
  ret = xlong(o) == 0 ? 0 : 1;

  Py_XDECREF(filt);
  Py_XDECREF(o);
  _LEAVE
  return ret;
}


int open(const char *pathname, int flags, ...)
{
  int fd;
  init();

  _ENTER(open, "\"%s\"", pathname)

  if (do_hook_open_pathname(pathname))
  {
    PyObject *o;
    PyObject *open_ = PyObject_GetAttrString(hooks, "open");

    o = pycall_args(open_, "(s)", pathname);

    fd = xlong(o);
    //Py_XDECREF(open_);
    //Py_XDECREF(o);
  }
  else
    fd = orig_open(pathname, flags);

  _LEAVE;
  return fd;
}


FILE *fopen(const char *pathname, const char *mode)
{
  int fd;
  int flags = 0;
  FILE *fp;

  init();

  _ENTER(fopen, "\"%s\"", pathname);
  //debug("++ fopen: %s (mode=%s)", pathname, mode);

  if (!strcmp(mode, "r"))
    flags = O_RDONLY;
  if (!strcmp(mode, "r+"))
    flags = O_RDWR;
  if (!strcmp(mode, "w"))
    flags = O_WRONLY | O_CREAT;
  if (!strcmp(mode, "w+"))
    flags = O_RDWR | O_CREAT | O_TRUNC;
  if (!strcmp(mode, "a"))
    flags = O_WRONLY | O_CREAT | O_TRUNC;
  if (!strcmp(mode, "a+"))
    flags = O_RDWR | O_CREAT;

  fd = open(pathname, flags);
  fp = fdopen(fd, mode);

  //debug("-- fopen %s\n", pathname);
  _LEAVE;
  return fp;
}

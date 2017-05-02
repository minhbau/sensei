#ifndef senseiPySequence_h
#define senseiPySequence_h

#include "senseiPyObject.h"
#include "Error.h"

#include <Python.h>
#include <vector>

// this macro is used to build up dispatchers
// PYT - type tag idnetifying the PyObject
// SEQ - PySequence* instance
// CODE - code to execute on match
// ST - typedef coresponding to matching tag
#define SENSEI_PY_SEQUENCE_DISPATCH_CASE(PYT, SEQ, CODE)  \
  if (senseiPySequence::is_type<PYT>(SEQ))                \
    {                                                     \
    using ST = PYT;                                       \
    CODE                                                  \
    }

// the macro dispatches for all the Python types
#define SENSEI_PY_SEQUENCE_DISPATCH(SEQ, CODE)            \
  SENSEI_PY_SEQUENCE_DISPATCH_CASE(bool, SEQ, CODE)       \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(int, SEQ, CODE)   \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(float, SEQ, CODE) \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(char*, SEQ, CODE) \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(long, SEQ, CODE)

// this one just the numeric types
#define SENSEI_PY_SEQUENCE_DISPATCH_NUM(SEQ, CODE)        \
  SENSEI_PY_SEQUENCE_DISPATCH_CASE(int, SEQ, CODE)        \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(float, SEQ, CODE) \
  else SENSEI_PY_SEQUENCE_DISPATCH_CASE(long, SEQ, CODE)

// this one just strings
#define SENSEI_PY_SEQUENCE_DISPATCH_STR(SEQ, CODE)  \
  SENSEI_PY_SEQUENCE_DISPATCH_CASE(char*, SEQ, CODE)


namespace senseiPySequence
{
// ****************************************************************************
template <typename py_t>
bool is_type(PyObject *seq)
{
  // nothing to do
  long n_items = PySequence_Size(seq);
  if (n_items < 1)
    return false;

  // all items must have same type and it must match
  // the requested type
  for (long i = 0; i < n_items; ++i)
    {
    if (!senseiPyObject::cpp_tt<py_t>::is_type(PySequence_GetItem(seq, i)))
      {
      if (i)
        {
        SENSEI_ERROR("Sequences with mixed types are not supported. "
          " Failed at element " <<  i)
        }
      return false;
      }
    }

  // sequence type matches
  return true;
}

// ****************************************************************************
template <typename cpp_t>
bool copy(cpp_t *va, unsigned long n, PyObject *seq)
{
  // not a sequence
  if (!PySequence_Check(seq) || PyString_Check(seq))
    return false;

  // nothing to do
  unsigned long n_items = PySequence_Size(seq);
  if (!n_items)
    return true;

  // check length of destination buffer
  if (n_items != n)
    {
    SENSEI_ERROR("destination buffer size(" << n
      << ") does not match source buffer size("
      << n_items << ")")
    return false;
    }

  // copy numeric types
  SENSEI_PY_SEQUENCE_DISPATCH_NUM(seq,
    for (unsigned long i = 0; i < n_items; ++i)
      {
      va[i] = senseiPyObject::cpp_tt<ST>::value(
        PySequence_GetItem(seq, i));
      }
    return true;
    )

  // unknown type, not an error, give other code chance to recognize it
  return false;
}

}

#endif
/**
 * Copyright (C) 2018 Daniel Turecek
 *
 * @file      commonpython.h
 * @author    Daniel Turecek <daniel@turecek.de>
 * @date      2018-09-17
 *
 */
#ifndef COMMONPYTHON_H
#define COMMONPYTHON_H

#ifdef WIN32
    #ifdef _WIN64
        #define PYK "K"
    #else
        #define PYK "I"
    #endif
#else
#define PYK "K"
#endif

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
    #define PyInt_Check PyLong_Check
    #define PyString_FromString PyUnicode_FromString
#endif


#endif /* !COMMONPYTHON_H */


/*  hunspell.cpp
 *
 *  Copyright (C) 2009 - Sayamindu Dasgupta <sayamindu@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Python.h>
#include <hunspell.hxx>


#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) PyObject_HEAD_INIT(type) size,
#endif

/* Compatibility python3 defines for python2 */
#if PY_MAJOR_VERSION < 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyBytes_FromString  PyString_FromString
#endif /* PY_MAJOR_VERSION < 3 */


/****************************************
                HunSpell
****************************************/

static PyObject *HunSpellError;

typedef struct {
    PyObject_HEAD
    Hunspell * handle;
    const char *encoding;
} HunSpell;

static int
HunSpell_init(HunSpell * self, PyObject *args, PyObject *kwds)
{
    PyObject *dpath = NULL; /* PyBytes in py3 PyString in py2 */
    PyObject *apath = NULL;
    FILE *fh;

#if PY_VERSION_HEX < 0x03010000
    /* TODO: Please review if there is any shorter/nicer;less clumsy way to convert args to  PyStrings using Py_FileSystemDefaultEncoding in python 2.x */
    const char * dpath_ptr = NULL;
    const char * apath_ptr = NULL;
    if (!PyArg_ParseTuple(args, "etet", Py_FileSystemDefaultEncoding, &dpath_ptr, Py_FileSystemDefaultEncoding, &apath_ptr))
        return 1;
    dpath = PyString_FromString(dpath_ptr);
    apath = PyString_FromString(apath_ptr);
#else
    if (!PyArg_ParseTuple(args, "O&O&", PyUnicode_FSConverter, &dpath, PyUnicode_FSConverter, &apath))
        return 1;
#endif
    /* Some versions of Hunspell() will succeed even if
    * there are no dictionary files. So test for permissions.
    */
    /* TODO: consider  _Py_fopen for py3.x here ? */
    fh = fopen(PyBytes_AsString(dpath), "r");
    if (fh) {
        fclose(fh);
    } else {
        PyErr_SetFromErrno(HunSpellError);
        /* TODO: Py_DECREF(*path); */
        return -1;
    }
    fh = fopen(PyBytes_AsString(apath), "r");
    if (fh) {
        fclose(fh);
    } else {
        PyErr_SetFromErrno(HunSpellError);
        return -1;
    }

    self->handle = new Hunspell(PyBytes_AsString(apath), PyBytes_AsString(dpath));
    // TODO check class instanciation went well
    //if(!self->handle) { Hunspell ain't bool
    //    PyErr_SetString(HunSpellError, "Cannot open dictionary");
    //    return -1;
    //}
    self->encoding = self->handle->get_dic_encoding();
    Py_DECREF(dpath);
    Py_DECREF(apath);
    return 0;
}

static void
HunSpell_dealloc(HunSpell * self)
{
    delete self->handle;
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
HunSpell_add_dic(HunSpell * self, PyObject *args, PyObject *kwds)
{
    PyObject *dpath = NULL; /* PyBytes in py3 PyString in py2 */
    FILE *fh;
#if PY_VERSION_HEX < 0x03010000
    const char * dpath_ptr = NULL;
    if (!PyArg_ParseTuple(args, "et", Py_FileSystemDefaultEncoding, &dpath_ptr))
        return NULL;
    dpath = PyString_FromString(dpath_ptr);
#else
    if (!PyArg_ParseTuple(args, "O&", PyUnicode_FSConverter, &dpath))
        return NULL;
#endif
    fh = fopen(PyBytes_AsString(dpath), "r");
    if (fh) {
        fclose(fh);
    } else {
        PyErr_SetFromErrno(HunSpellError);
        Py_DECREF(dpath);
        return NULL;
    }
    int result = self->handle->add_dic(PyBytes_AsString(dpath));
    Py_DECREF(dpath);
    return PyLong_FromLong(result);
}

static PyObject *
HunSpell_get_dic_encoding(HunSpell * self, PyObject *args)
{
    return Py_BuildValue("s", self->encoding);
}

static PyObject *
HunSpell_spell(HunSpell * self, PyObject *args)
{
    char *word;
    int retvalue;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
        return NULL;
    retvalue = self->handle->spell(word);
    PyMem_Free(word);
    return PyBool_FromLong(retvalue);
}


static PyObject *
HunSpell_suggest(HunSpell * self, PyObject *args)
{
    char *word, **slist;
    int i, num_slist, ret;
    PyObject *slist_list, *pystr;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
        return NULL;

    slist_list = PyList_New(0);
    if (!slist_list) {
        return NULL;
    }
    num_slist = self->handle->suggest(&slist, word);
    PyMem_Free(word);

    for (i = 0, ret = 0; !ret && i < num_slist; i++) {
        pystr = PyUnicode_FromString(slist[i]);
        if (!pystr)
            break;
        ret = PyList_Append(slist_list, pystr);
        Py_DECREF(pystr);
    }

    self->handle->free_list(&slist, num_slist);
    return slist_list;
}

static PyObject *
HunSpell_analyze(HunSpell * self, PyObject *args)
{
    char *word, **slist;
    int i, num_slist, ret;
    PyObject *slist_list, *pystr;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
        return NULL;

    slist_list = PyList_New(0);
    if (!slist_list) {
        return NULL;
    }
    num_slist = self->handle->analyze(&slist, word);
    PyMem_Free(word);

    for (i = 0, ret = 0; !ret && i < num_slist; i++) {
        pystr = PyBytes_FromString(slist[i]);
        if (!pystr)
            break;
        ret = PyList_Append(slist_list, pystr);
        Py_DECREF(pystr);
    }

    self->handle->free_list(&slist, num_slist);
    return slist_list;
}

static PyObject *
HunSpell_stem(HunSpell * self, PyObject *args)
{
    char *word, **slist;
    int i, num_slist, ret;
    PyObject *slist_list, *pystr;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
    return NULL;

    slist_list = PyList_New(0);
    if (!slist_list) {
        return NULL;
    }
    num_slist = self->handle->stem(&slist, word);
    PyMem_Free(word);

    for (i = 0, ret = 0; !ret && i < num_slist; i++) {
        pystr = PyBytes_FromString(slist[i]);
        if (!pystr)
            break;
        ret = PyList_Append(slist_list, pystr);
        Py_DECREF(pystr);
    }

    self->handle->free_list(&slist, num_slist);
    return slist_list;
}

static PyObject *
HunSpell_generate(HunSpell * self, PyObject *args)
{
    char *word1, *word2, **slist;
    int i, num_slist, ret;
    PyObject *slist_list, *pystr;

    if (!PyArg_ParseTuple(args, "etet", self->encoding, &word1, self->encoding, &word2))
    return NULL;

    slist_list = PyList_New(0);
    if (!slist_list) {
        return NULL;
    }
    num_slist = self->handle->generate(&slist, word1, word2);
    PyMem_Free(word1);
    PyMem_Free(word2);

    for (i = 0, ret = 0; !ret && i < num_slist; i++) {
        pystr = PyBytes_FromString(slist[i]);
        if (!pystr)
            break;
        ret = PyList_Append(slist_list, pystr);
        Py_DECREF(pystr);
    }

    self->handle->free_list(&slist, num_slist);
    return slist_list;
}

static PyObject *
HunSpell_generate2(HunSpell * self, PyObject *args)
{
    char *word1, *desc, **slist;
    int i, num_slist, ret;
    PyObject *slist_list, *pystr;

    if (!PyArg_ParseTuple(args, "etet", self->encoding, &word1, self->encoding, &desc))
        return NULL;

    slist_list = PyList_New(0);
    if (!slist_list) {
        return NULL;
    }

    num_slist = self->handle->generate(&slist, word1, &desc, 1);
    PyMem_Free(word1);
    PyMem_Free(desc);

    for (i = 0, ret = 0; !ret && i < num_slist; i++) {
        pystr = PyBytes_FromString(slist[i]);
        if (!pystr)
            break;
        ret = PyList_Append(slist_list, pystr);
        Py_DECREF(pystr);
    }

    self->handle->free_list(&slist, num_slist);
    return slist_list;
}

static PyObject *
HunSpell_add(HunSpell * self, PyObject *args)
{
    char *word;
    int retvalue;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
        return NULL;
    retvalue = self->handle->add(word);
    PyMem_Free(word);

    return PyLong_FromLong(retvalue);
}

static PyObject *
HunSpell_add_with_affix(HunSpell * self, PyObject *args)
{
    char *word, *example;
    int retvalue;

    if (!PyArg_ParseTuple(args, "etet", self->encoding, &word, self->encoding, &example))
        return NULL;
    retvalue = self->handle->add_with_affix(word, example);
    PyMem_Free(word);
    PyMem_Free(example);

    return PyLong_FromLong(retvalue);
}

static PyObject *
HunSpell_remove(HunSpell * self, PyObject *args)
{
    char *word;
    int retvalue;

    if (!PyArg_ParseTuple(args, "et", self->encoding, &word))
        return NULL;
    retvalue = self->handle->remove(word);
    PyMem_Free(word);

    return PyLong_FromLong(retvalue);
}

static PyMethodDef HunSpell_methods[] = {
    {"get_dic_encoding", (PyCFunction) HunSpell_get_dic_encoding,
     METH_NOARGS,
     "Gets encoding of loaded dictionary.\n\n"
     "Returns\n"
     "-------\n"
     "string : The encoding of currently used dic file (UTF-8, ISO8859-1, ...)"},

    {"add_dic", (PyCFunction) HunSpell_add_dic, METH_VARARGS,
     "Load an extra dictionary to the current instance.\n"
     "The  extra dictionaries use the affix file of the allocated Hunspell object.\n"
     "Maximal number of the extra dictionaries is limited in the Hunspell source code to 20.\n\n"
     "Parameters\n"
     "----------\n"
     "dpath : string\n"
     "    Path to the .dic to add.\n\n"
     "Returns\n"
     "-------\n"
     "int : hunspell program error code."},

    {"spell", (PyCFunction) HunSpell_spell, METH_VARARGS,
     "Checks the spelling of the given word.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    Word to check.\n\n"
     "Returns\n"
     "-------\n"
     "bool : True if the word is correctly spelled else False"},

    {"suggest", (PyCFunction) HunSpell_suggest, METH_VARARGS,
     "Provide suggestions for the given word.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    Word for which we want suggestions\n\n"
     "Returns\n"
     "-------\n"
     "list of strings : The list of suggestions for input word. (No suggestion returns an empty list)."},

    {"analyze", (PyCFunction) HunSpell_analyze, METH_VARARGS,
     "Provide morphological analysis for the given word.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    Input word to analyze.\n\n"
     "Returns\n"
     "-------\n"
     "list of strings : Each string is a possible analysis of the input word. "
     "It contains the stem of the word (st:XXX) and some information about "
     "modifications done to get to the input word.\n"
     "For more information see: man 4 hunspell (or https://sourceforge.net/projects/hunspell/files/Hunspell/Documentation/) "
     "in the \'Optional data fields\" section."},

    {"stem", (PyCFunction) HunSpell_stem, METH_VARARGS,
     "Stemmer method. It is a simplified version of analyze method.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to stem.\n\n"
     "Returns\n"
     "-------\n"
     "list of string : The possible stems of the input word."},

    {"generate", (PyCFunction) HunSpell_generate, METH_VARARGS,
     "Provide morphological generation for the given word using "
     "the second one as example.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to transform.\n"
     "word : string\n"
     "    The example to use as a generator\n\n"
     "Returns\n"
     "-------\n"
     "list of string : A list of possible transformations or "
     "an empty list if nothing were found"},

    {"generate2", (PyCFunction) HunSpell_generate2, METH_VARARGS,
     "Provide morphological generation for the given word "
     "the second one as example.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to transform.\n"
     "tags : string\n"
     "    String of an analyzed word\n\n"
     "Returns\n"
     "-------\n"
     "list of string : A list of possible transformations or "
     "an empty list if nothing were found"},

    {"add", (PyCFunction) HunSpell_add, METH_VARARGS,
     "Adds the given word into the runtime dictionary.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to add in the dictionary\n\n"
     "Returns\n"
     "-------\n"
     "int : 0 if success, hunspell program error code else."},

    {"add_with_affix", (PyCFunction) HunSpell_add_with_affix, METH_VARARGS,
     "Adds the given word with affix flags of the example (a dictionary word) "
     "into the runtime dictionary.\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to transform.\n"
     "word : string\n"
     "    The example to use to find flags\n\n"
     "Returns\n"
     "-------\n"
     "int : 0 if success, hunspell program error code else."},

    {"remove", (PyCFunction) HunSpell_remove, METH_VARARGS,
     "Removes the given word from the runtime dictionary\n\n"
     "Parameters\n"
     "----------\n"
     "word : string\n"
     "    The word to remove from the dictionary\n\n"
     "Returns\n"
     "-------\n"
     "int : 0 if success, hunspell program error code else."},

    {NULL}
};

static PyTypeObject HunSpellType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "HunSpell",        /* tp_name */
    sizeof(HunSpell),    /* tp_basicsize */
    0,            /* tp_itemsize */
    (destructor) HunSpell_dealloc,    /* tp_dealloc */
    0,            /* tp_print */
    0,            /* tp_getattr */
    0,            /* tp_setattr */
    0,            /* tp_compare */
    0,            /* tp_repr */
    0,            /* tp_as_number */
    0,            /* tp_as_sequence */
    0,            /* tp_as_mapping */
    0,            /* tp_hash */
    0,            /* tp_call */
    0,            /* tp_str */
    0,            /* tp_getattro */
    0,            /* tp_setattro */
    0,            /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,    /* tp_flags */
    "HunSpell binding. \n\n"
    "Instantiation goes like this:\n"
    ">>> hobj = HunSpell('/path/to/dict.dic', '/path/to/dict.aff')",    /* tp_doc */
    0,            /* tp_traverse */
    0,            /* tp_clear */
    0,            /* tp_richcompare */
    0,            /* tp_weaklistoffset */
    0,            /* tp_iter */
    0,            /* tp_iternext */
    HunSpell_methods,    /* tp_methods */
    0,            /* tp_members */
    0,            /* tp_getset */
    0,            /* tp_base */
    0,            /* tp_dict */
    0,            /* tp_descr_get */
    0,            /* tp_descr_set */
    0,            /* tp_dictoffset */
    (initproc) HunSpell_init,    /* tp_init */
    0,            /* tp_alloc */
    0,            /* tp_new */
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef hunspellmodule = {
    PyModuleDef_HEAD_INIT,
    "hunspell",    /* name of module */
    NULL,        /* module documentation, may be NULL */
    -1, /* TODO */    /* size of per-interpreter state of the module,
               or -1 if the module keeps state in global variables. */
    HunSpell_methods
};
#endif

/******************** Module Initialization function ****************/

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC
PyInit_hunspell(void)
{
    PyObject *mod;
    mod = PyModule_Create(&hunspellmodule);
#else
extern "C" PyObject*
inithunspell(void)
{
    PyObject *mod;
    mod = Py_InitModule3("hunspell", NULL,
                         "An extension for the Hunspell spell checker engine");
#endif
    if (mod == NULL) {
        return NULL;
    }
    /* Fill in some slots in the type, and make it ready */
    HunSpellType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&HunSpellType) < 0) {
        return NULL;
    }
    /* Add the type to the module. */
    Py_INCREF(&HunSpellType);
    PyModule_AddObject(mod, "HunSpell", (PyObject *)&HunSpellType);
    HunSpellError = PyErr_NewException((char*) "hunspell.HunSpellError", NULL, NULL);
    Py_INCREF(HunSpellError);
    PyModule_AddObject(mod, "HunSpellError", HunSpellError);
    return mod;
}

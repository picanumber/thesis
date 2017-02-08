#include <iostream>

#include "include.h"
#include "optimization.h"

static int get_split_type(const char *split, int& splitType)
{
	int ret(0); 
	if (!strcmp(split, "lin"))
	{
		splitType = LOAD_LIN;
	}
	else if (!strcmp(split, "qdrt"))
	{
		splitType = LOAD_QDRT;
	}
	else if (!strcmp(split, "rstar"))
	{
		splitType = LOAD_RSTAR;
	}
	else if (!strcmp(split, "bulk"))
	{
		splitType = LOAD_BULK;
	}
	else
	{
		ret = 1; 
	}
	return ret; 
}

static PyObject* objective_function(PyObject *self, PyObject *args, PyObject *keywds)
{
	int dataset, numElems, numQs, qType, minNodes, maxNodes, splitType(0); 
	//const char *split;
	int split;

	static char *kwlist[] = { 
		"dataset", 
		"numElems", 
		"numQs", 
		"qType", 
		"minNodes", 
		"maxNodes", 
		"splitType", 
		NULL 
	};

	if (!PyArg_ParseTupleAndKeywords(
		//args, keywds, "iiiiiis", kwlist,
		args, keywds, "iiiiiii", kwlist,
		&dataset, &numElems, &numQs, &qType, &minNodes, &maxNodes, &split))
	{
		return NULL; // TODO: should I just Py_RETURN_NONE here?
	}

	//if (get_split_type(split, splitType))
	if ((splitType = split) > LOAD_BULK || splitType < LOAD_LIN)
	{
		return PyLong_FromLong(-1);
	}

	return PyLong_FromLong((long)run_rtree_operations(
		dataset, numElems, numQs, qType, minNodes, maxNodes, splitType));

	//Py_RETURN_NONE;
}

PyMethodDef OptimizerMethods[] = 
{
	{ 
		"objective_function", (PyCFunction)objective_function, 
		METH_VARARGS | METH_KEYWORDS, "Definition of the objective function"
	},
	{ NULL, NULL, 0, NULL } // sentinel 
}; 


PyMODINIT_FUNC
initoptimize_rtree(void)
{
	PyObject *m;

	m = Py_InitModule("optimize_rtree", OptimizerMethods);
	if (m == NULL)
		return;

	//SpamError = PyErr_NewException("spam.error", NULL, NULL);
	//Py_INCREF(SpamError);
	//PyModule_AddObject(m, "error", SpamError);
}




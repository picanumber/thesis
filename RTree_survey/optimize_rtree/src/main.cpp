#include <iostream>

#include "include.h"
#include "optimization.h"

static PyObject* objective_function(PyObject *self, PyObject *args, PyObject *keywds)
{
	int dataset, numElems, numQs, qType, minNodes, maxNodes, splitType; 

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
		args, keywds, "iiiiiii", kwlist, 
		&dataset, &numElems, &numQs, &qType, &minNodes, &maxNodes, &splitType))
	{
		return NULL; // TODO: should I just Py_RETURN_NONE here?
	}

	std::cout << "Heeee Hooooo\n\n"; 

	return PyLong_FromLong((long)run_rtree_operations(
		dataset, numElems, numQs, qType, minNodes, maxNodes, splitType));

	Py_RETURN_NONE;
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




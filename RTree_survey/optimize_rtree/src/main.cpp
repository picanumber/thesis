#include "include.h"
#include "optimization.h"

static PyObject* objective_function(PyObject *self, PyObject *args, PyObject *keywds)
{
	int voltage;
	char *state = "a stiff";
	char *action = "voom";
	char *type = "Norwegian Blue";

	static char *kwlist[] = { "voltage", "state", "action", "type", NULL };

	if (!PyArg_ParseTupleAndKeywords(
		args, keywds, "i|sss", kwlist, &voltage, &state, &action, &type))
	{ 
		return NULL; // TODO: should I just Py_RETURN_NONE here?
	}

	printf("-- This parrot wouldn't %s if you put %i Volts through it.\n",
		action, voltage);
	printf("-- Lovely plumage, the %s -- It's %s!\n", type, state);

	//return PyFloat_FromDouble((double)test_optimizer(voltage, state, action, type)); 

	Py_RETURN_NONE;
}

PyMethodDef OptRtreeMethods[] = 
{
	{ "objective_function", (PyCFunction)objective_function, METH_VARARGS | METH_KEYWORDS, "Definition of the objective function" },
	{ NULL, NULL, 0, NULL } // sentinel 
}; 

static struct PyModuleDef optimize_rtreemodule = 
{
	PyModuleDef_HEAD_INIT,
	"optimize_rtree",                   /* name of module */
	"Optimization of rtree parameters", /* module documentation, may be NULL */
	-1,                                 /* size of per-interpreter state of the module,
			                            or -1 if the module keeps state in global variables. */
	OptRtreeMethods
};

PyMODINIT_FUNC PyInit_optimize_rtree(void)
{
	return PyModule_Create(&optimize_rtreemodule);
}

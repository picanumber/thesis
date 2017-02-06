#include <string>
#include "bmk_utils.h"
#include <algorithm>
#include <functional>
#include <CODEine/benchmark.h>

template <class It>
int stringifier(It first, It last, std::vector<std::string>::iterator out)
{
	while (first != last)
	{
		*out++ = std::to_string(*first);
		++first;
	}
	return 0;
}

int _main()
{
	//ccleaner obj;   
	//auto k = obj(); 
	//std::cout << k << std::endl;
	int k;
	std::cout << "waiting for you to set the affinity\n";
	std::cin >> k;

	std::vector<int> v(10 * 1024 * 1024);
	std::vector<std::string> vo(v.size());

	utl::detail::random_generator<int> rg(10, 10'000'000);
	std::generate(v.begin(), v.end(), std::ref(rg));

	bmk::benchmark<> bm;

	bm.run(
		"to_string", 10, [&](int num) {
		auto first = v.begin();
		auto last = v.begin() + num;
		stringifier(first, last, vo.begin());
	},
		"number of conversions",
		{ 100'000, 200'000, 300'000, 400'000, 500'000, 600'000, 700'000, 800'000, 900'000, 1'000'000 }
	);


	bm.serialize("convert num 2 str", "test.txt");

	return 0;
}


#if 0
// exporting from cpp for Python3
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
#endif // 0

#pragma once

#include <type_traits>

namespace utl
{
	/// http://foonathan.net/blog/2017/01/20/function-ref-implementation.html 
    /// https://github.com/llvm-mirror/llvm/blob/master/include/llvm/ADT/STLExtras.h#L80

	/// An efficient, type-erasing, non-owning reference to a callable. This is
	/// intended for use as the type of a function parameter that is not used
	/// after the function in question returns.
	///
	/// This class does not own the callable, so it is not in general safe to store
	/// a function_ref.
	template<typename Fn> class function_ref;

	template<typename Ret, typename ...Params>
	class function_ref<Ret(Params...)>
	{
		Ret(*callback)(intptr_t callable, Params ...params);
		intptr_t callable;

		template<typename Callable>
		static Ret callback_fn(intptr_t callable, Params ...params)
		{
			return (*reinterpret_cast<Callable*>(callable))(
				std::forward<Params>(params)...);
		}

	public:
		template <typename Callable>
		function_ref(
			Callable &&callable,
			std::enable_if_t<
			!std::is_same<typename std::remove_reference<Callable>::type,
			function_ref>::value>* = nullptr
		)
			: callback(callback_fn<typename std::remove_reference<Callable>::type>)
			, callable(reinterpret_cast<intptr_t>(&callable))
		{
		}

		Ret operator()(Params ...params) const
		{
			return callback(callable, std::forward<Params>(params)...);
		}
	};
}


#ifndef SUPERSDL_UTIL_HPP
#define SUPERSDL_UTIL_HPP

#include <SDL_stdinc.h>
#include <SDL_video.h>
#include <memory>
#include <system_error>
#include <type_traits>
#include <utility>

namespace sps::util {

template <typename Creator, typename Destructor, typename... Arguments>
auto makeResource(Creator c, Destructor d, Arguments &&... args) {
	auto r = c(std::forward<Arguments>(args)...);
	if (!r) {
		throw std::system_error(errno, std::generic_category());
	}
	return std::unique_ptr<std::decay_t<decltype(*r)>, decltype(d)>(r, d);
}

using window_ptr_t = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using sdl_str_t = std::unique_ptr<char[], decltype(&SDL_free)>; 

} // namespace sps::util

#endif

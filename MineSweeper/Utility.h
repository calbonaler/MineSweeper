#pragma once

#include <iterator>
#include <sstream>
#include <iomanip>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#define LVALUE_REF &
#define RVALUE_REF &&

template <typename Container> class back_emplace_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
	explicit back_emplace_iterator(Container& container) : container(&container) { }
	back_emplace_iterator(const back_emplace_iterator& source) : container(source.container) { }
	back_emplace_iterator(back_emplace_iterator&& source) : container(source.container) { source.container = nullptr; }
	back_emplace_iterator& operator =(const back_emplace_iterator& source)
	{
		container = source.container;
		return *this;
	}
	back_emplace_iterator& operator =(back_emplace_iterator&& source)
	{
		std::swap(container, source.container);
		return *this;
	}

	template <typename T> back_emplace_iterator& operator =(T&& t)
	{
		container->emplace_back(std::forward<T>(t));
		return *this;
	}
	back_emplace_iterator& operator*() { return *this; }
	back_emplace_iterator& operator++() { return *this; }
	back_emplace_iterator& operator++(int) { return *this; }

protected:
	Container* container;
};

template <typename Container> inline back_emplace_iterator<Container> back_emplacer(Container& c) { return back_emplace_iterator<Container>(c); }

template <typename T> class simple_set_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
public:
	explicit simple_set_iterator(T& storage) : storage(&storage) { }

	T& operator*() { return *storage; }
	simple_set_iterator& operator++() { return *this; }
	simple_set_iterator& operator++(int) { return *this; }

protected:
	T* storage;
};

template <typename T> inline simple_set_iterator<T> simple_setter(T& storage) { return simple_set_iterator<T>(storage); }

inline void ThrowLastException()
{
	auto errorCode = GetLastError();
	void* buffer;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), static_cast<CHAR*>(static_cast<void*>(&buffer)), 0, nullptr);
	std::stringstream ss;
	ss << static_cast<const CHAR*>(buffer) << " (" << std::setfill('0') << std::setw(8) << std::hex << errorCode << ")";
	LocalFree(buffer);
	throw std::runtime_error(ss.str());
}

template <typename T> inline typename std::remove_reference<T>::type ThrowIfFailed(T&& value)
{
	if (!value) ThrowLastException();
	return std::forward<T>(value);
}

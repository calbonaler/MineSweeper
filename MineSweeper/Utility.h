#pragma once

#include <iterator>
#include <sstream>
#include <iomanip>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

template <typename T> class LocalPointer {
public:
	LocalPointer() : p(nullptr) {}
	LocalPointer(const LocalPointer&) = delete;
	LocalPointer(LocalPointer&& src) noexcept : p(src.p) { src.p = nullptr; }
	LocalPointer& operator =(const LocalPointer&) = delete;
	LocalPointer& operator =(LocalPointer&& src) noexcept { std::swap(p, src.p); }
	~LocalPointer() {
		LocalFree(p);
		p = nullptr;
	}

	T** GetAddress() { return &p; }
	T* const* GetAddress() const { return &p; }
	T* Get() const { return p; }

private:
	T* p;
};

inline void ThrowLastException()
{
	auto errorCode = GetLastError();
	LocalPointer<CHAR> buffer;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), static_cast<CHAR*>(static_cast<void*>(buffer.GetAddress())), 0, nullptr);
	std::stringstream ss;
	ss << buffer.Get() << " (" << std::setfill('0') << std::setw(8) << std::hex << errorCode << ")";
	throw std::runtime_error(ss.str());
}

template <typename T> inline typename std::remove_reference<T>::type ThrowIfFailed(T&& value)
{
	if (!value) ThrowLastException();
	return std::forward<T>(value);
}

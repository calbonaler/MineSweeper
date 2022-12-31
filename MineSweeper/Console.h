#pragma once

#include <string>
#include <optional>
#include <variant>
#include "Utility.h"

enum class ConsoleColor : uint8_t
{
	Black = 0,
	Navy = 1,
	Green = 2,
	Teal = 3,
	Maroon = 4,
	Purple = 5,
	Olive = 6,
	Silver = 7,
	Gray = 8,
	Blue = 9,
	Lime = 10,
	Cyan = 11,
	Red = 12,
	Magenta = 13,
	Yellow = 14,
	White = 15,
};

constexpr uint8_t ConsoleColorMask = static_cast<uint8_t>(ConsoleColor::White);

struct ConsoleCharacterAttribute
{
	constexpr ConsoleCharacterAttribute() : Foreground(ConsoleColor::Black), Background(ConsoleColor::Black) { }
	constexpr ConsoleCharacterAttribute(ConsoleColor foreground, ConsoleColor background) : Foreground(foreground), Background(background) { }
	constexpr explicit ConsoleCharacterAttribute(uint16_t value) : Foreground(static_cast<ConsoleColor>(value & ConsoleColorMask)), Background(static_cast<ConsoleColor>((value << 4) & ConsoleColorMask)) { }

	ConsoleColor Foreground;
	ConsoleColor Background;

	constexpr explicit operator uint16_t() const { return static_cast<uint16_t>(Foreground) | static_cast<uint16_t>(Background) << 4; }
};

struct ConsoleCoordinate
{
	constexpr ConsoleCoordinate() : X(0), Y(0) { }
	constexpr ConsoleCoordinate(int16_t x, int16_t y) : X(x), Y(y) { }
	constexpr explicit ConsoleCoordinate(COORD value) : X(value.X), Y(value.Y) { }

	int16_t X;
	int16_t Y;

	constexpr explicit operator COORD() const { return { X, Y }; }
};

struct ConsoleSize
{
	constexpr ConsoleSize() : Width(0), Height(0) { }
	constexpr ConsoleSize(int16_t width, int16_t height) : Width(width), Height(height) { }
	constexpr explicit ConsoleSize(COORD value) : Width(value.X), Height(value.Y) { }

	int16_t Width;
	int16_t Height;

	constexpr explicit operator COORD() const { return { Width, Height }; }
};

struct ConsoleRect
{
	constexpr ConsoleRect() : Left(0), Top(0), Right(0), Bottom(0) { }
	constexpr ConsoleRect(int16_t left, int16_t top, int16_t right, int16_t bottom) : Left(left), Top(top), Right(right), Bottom(bottom) { }
	constexpr explicit ConsoleRect(SMALL_RECT value) : Left(value.Left), Top(value.Top), Right(value.Right), Bottom(value.Bottom) { }

	int16_t Left;
	int16_t Top;
	int16_t Right;
	int16_t Bottom;

	constexpr explicit operator SMALL_RECT() const { return { Left, Top, Right, Bottom }; }
};

enum class FontFamilyType : uint8_t 
{
	DontCare = 0,
	Roman = 1,
	Swiss = 2,
	Modern = 3,
	Script = 4,
	Decorative = 5,
};

struct ConsoleFontInfo
{
	constexpr ConsoleFontInfo() : Index(0), PitchAndFamily(0), Weight(0) { }
	constexpr explicit ConsoleFontInfo(const CONSOLE_FONT_INFOEX& value) : Index(value.nFont), Size(value.dwFontSize), PitchAndFamily(value.FontFamily), Weight(value.FontWeight), FaceName(value.FaceName) { }

	uint32_t Index;
	ConsoleSize Size;
	uint32_t PitchAndFamily;
	uint32_t Weight;
	std::wstring FaceName;

	constexpr bool GetIsFixedPitch() const { return GetFlag(PitchAndFamily, TMPF_FIXED_PITCH); }
	constexpr void SetIsFixedPitch(bool value) { SetFlag(PitchAndFamily, TMPF_FIXED_PITCH, value); }
	constexpr bool GetIsVector() const { return GetFlag(PitchAndFamily, TMPF_VECTOR); }
	constexpr void SetIsVector(bool value) { SetFlag(PitchAndFamily, TMPF_VECTOR, value); }
	constexpr bool GetIsTrueType() const { return GetFlag(PitchAndFamily, TMPF_TRUETYPE); }
	constexpr void SetIsTrueType(bool value) { SetFlag(PitchAndFamily, TMPF_TRUETYPE, value); }
	constexpr bool GetIsDevice() const { return GetFlag(PitchAndFamily, TMPF_DEVICE); }
	constexpr void SetIsDevice(bool value) { SetFlag(PitchAndFamily, TMPF_DEVICE, value); }
	constexpr FontFamilyType GetFamilyType() const { return static_cast<FontFamilyType>(PitchAndFamily >> 4); }
	constexpr void SetFamilyType(FontFamilyType value) { PitchAndFamily = PitchAndFamily & 0xF | static_cast<uint32_t>(value) << 4; }

	constexpr explicit operator CONSOLE_FONT_INFOEX() const
	{
		CONSOLE_FONT_INFOEX info;
		CopyTo(info);
		return info;
	}
	constexpr void CopyTo(CONSOLE_FONT_INFOEX& info) const
	{
		info.cbSize = sizeof(CONSOLE_FONT_INFOEX);
		info.nFont = Index;
		info.dwFontSize = static_cast<COORD>(Size);
		info.FontFamily = PitchAndFamily;
		info.FontWeight = Weight;
		auto size = FaceName.copy(info.FaceName, sizeof(info.FaceName) / sizeof(info.FaceName[0]) - 1);
		info.FaceName[size] = L'\0';
	}

private:
	template <typename T, typename TValue> constexpr static bool GetFlag(T storage, TValue flag) { return storage & static_cast<T>(flag); }
	template <typename T, typename TValue> constexpr static void SetFlag(T& storage, TValue flag, bool set)
	{
		storage &= ~static_cast<T>(flag);
		if (set) storage |= static_cast<T>(flag);
	}
};

#define ALLOW_ENUM_FLAG_OPERATIONS(EnumType) \
	constexpr inline EnumType operator ~(EnumType value) noexcept { return static_cast<EnumType>(~static_cast<std::underlying_type<EnumType>::type>(value)); } \
	constexpr inline EnumType operator &(EnumType left, EnumType right) noexcept { return static_cast<EnumType>(static_cast<std::underlying_type<EnumType>::type>(left) & static_cast<std::underlying_type<EnumType>::type>(right)); } \
	constexpr inline EnumType operator ^(EnumType left, EnumType right) noexcept { return static_cast<EnumType>(static_cast<std::underlying_type<EnumType>::type>(left) ^ static_cast<std::underlying_type<EnumType>::type>(right)); } \
	constexpr inline EnumType operator |(EnumType left, EnumType right) noexcept { return static_cast<EnumType>(static_cast<std::underlying_type<EnumType>::type>(left) | static_cast<std::underlying_type<EnumType>::type>(right)); } \
	constexpr inline EnumType& operator &=(EnumType& left, EnumType right) noexcept { return left = left & right; } \
	constexpr inline EnumType& operator ^=(EnumType& left, EnumType right) noexcept { return left = left ^ right; } \
	constexpr inline EnumType& operator |=(EnumType& left, EnumType right) noexcept { return left = left | right; }

enum class ConsoleOutputModes : uint32_t
{
	Default = 0,
	EnableProcessedOutput = ENABLE_PROCESSED_OUTPUT,
	EnableWrapAtEolOutput = ENABLE_WRAP_AT_EOL_OUTPUT,
	EnableVirtualTerminalProcessing = ENABLE_VIRTUAL_TERMINAL_PROCESSING,
	DisableNewLineAutoReturn = DISABLE_NEWLINE_AUTO_RETURN,
	EnableLvbGridWorldWide = ENABLE_LVB_GRID_WORLDWIDE,
};
ALLOW_ENUM_FLAG_OPERATIONS(ConsoleOutputModes)

enum class ConsoleInputModes : uint32_t
{
	Default = 0,
	EnableProcessedInput = ENABLE_PROCESSED_INPUT,
	EnableLineInput = ENABLE_LINE_INPUT,
	EnableEchoInput = ENABLE_ECHO_INPUT,
	EnableWindowInput = ENABLE_WINDOW_INPUT,
	EnableMouseInput = ENABLE_MOUSE_INPUT,
	EnableInsertMode = ENABLE_INSERT_MODE,
	EnableQuickEditMode = ENABLE_QUICK_EDIT_MODE,
	EnableExtendedFlags = ENABLE_EXTENDED_FLAGS,
	EnableAutoPosition = ENABLE_AUTO_POSITION,
	EnableVirtualTerminalInput = ENABLE_VIRTUAL_TERMINAL_INPUT,
};
ALLOW_ENUM_FLAG_OPERATIONS(ConsoleInputModes)

enum class ConsoleControlKeyStates : uint32_t
{
	None = 0,
	RightAlt = RIGHT_ALT_PRESSED,
	LeftAlt = LEFT_ALT_PRESSED,
	RightCtrl = RIGHT_CTRL_PRESSED,
	LeftCtrl = LEFT_CTRL_PRESSED,
	Shift = SHIFT_PRESSED,
	NumLock = NUMLOCK_ON,
	ScrollLock = SCROLLLOCK_ON,
	CapsLock = CAPSLOCK_ON,
	EnhancedKey = ENHANCED_KEY,
	DbcsChar = NLS_DBCSCHAR,
	AlphaNumeric = NLS_ALPHANUMERIC,
	Katakana = NLS_KATAKANA,
	Hiragana = NLS_HIRAGANA,
	Roman = NLS_ROMAN,
	ImeConversion = NLS_IME_CONVERSION,
	ImeDisable = NLS_IME_DISABLE,
};
ALLOW_ENUM_FLAG_OPERATIONS(ConsoleControlKeyStates)

struct KeyEventRecord
{
public:
	constexpr KeyEventRecord() : IsKeyDown(false), RepeatCount(0), VirtualKeyCode(0), VirtualScanCode(0), Char(L'\0'), ControlKeyState(ConsoleControlKeyStates::None) { }
	constexpr KeyEventRecord(const KEY_EVENT_RECORD& record) :
		IsKeyDown(record.bKeyDown),
		RepeatCount(record.wRepeatCount),
		VirtualKeyCode(record.wVirtualKeyCode),
		VirtualScanCode(record.wVirtualScanCode),
		Char(record.uChar.UnicodeChar),
		ControlKeyState(static_cast<ConsoleControlKeyStates>(record.dwControlKeyState)) { }

	bool IsKeyDown;
	uint16_t RepeatCount;
	uint16_t VirtualKeyCode;
	uint16_t VirtualScanCode;
	WCHAR Char;
	ConsoleControlKeyStates ControlKeyState;
};

enum class MouseEventKind : uint32_t
{
	PressedOrReleased = 0,
	Moved = MOUSE_MOVED,
	DoubleClicked = DOUBLE_CLICK,
	VerticallyWheeled = MOUSE_WHEELED,
	HorizontallyWheeled = MOUSE_HWHEELED,
};

struct MouseButtonState
{
	constexpr MouseButtonState() : m_Value(0) { }
	constexpr explicit MouseButtonState(uint16_t value) : m_Value(value) { }
	constexpr bool GetLeft(uint8_t index = 0) const
	{
		CheckIndex(index);
		return At(index);
	}
	constexpr bool GetRight(uint8_t index = 0) const
	{
		CheckIndex(index);
		return At(15 - index);
	}
	constexpr bool operator ==(const MouseButtonState& right) const { return m_Value == right.m_Value; }
	constexpr bool operator !=(const MouseButtonState& right) const { return !(*this == right); }
	constexpr operator bool() const { return m_Value != 0; }

	constexpr MouseButtonState& operator &=(const MouseButtonState& right)
	{
		m_Value &= right.m_Value;
		return *this;
	}
	constexpr MouseButtonState& operator ^=(const MouseButtonState& right)
	{
		m_Value ^= right.m_Value;
		return *this;
	}
	constexpr MouseButtonState& operator |=(const MouseButtonState& right)
	{
		m_Value |= right.m_Value;
		return *this;
	}
	constexpr MouseButtonState& Flip()
	{
		m_Value = ~m_Value;
		return *this;
	}
	constexpr MouseButtonState operator &(const MouseButtonState& right) const { return MouseButtonState(m_Value & right.m_Value); }
	constexpr MouseButtonState operator ^(const MouseButtonState& right) const { return MouseButtonState(m_Value ^ right.m_Value); }
	constexpr MouseButtonState operator |(const MouseButtonState& right) const { return MouseButtonState(m_Value | right.m_Value); }
	constexpr MouseButtonState operator ~() const { return MouseButtonState(~m_Value); }

private:
	uint16_t m_Value;

	constexpr void CheckIndex(uint8_t index) const
	{
		if (index >= 16)
			throw std::out_of_range("Index must be less than 16.");
	}
	constexpr bool At(uint8_t index) const
	{
		if (index == 0)
			return m_Value & 1u;
		else if (index == 15)
			return m_Value & 2u;
		else
			return m_Value & (1u << (index + 1u));
	}
};

struct MouseEventRecord
{
	constexpr MouseEventRecord() : Location(), ButtonState(), Delta(0), ControlKeyState(ConsoleControlKeyStates::None), Kind(MouseEventKind::PressedOrReleased) { }
	constexpr MouseEventRecord(const MOUSE_EVENT_RECORD& record) :
		Location(record.dwMousePosition),
		ButtonState(record.dwButtonState & 0xFFFF),
		Delta(static_cast<int16_t>(record.dwButtonState >> 16)),
		ControlKeyState(static_cast<ConsoleControlKeyStates>(record.dwControlKeyState)),
		Kind(static_cast<MouseEventKind>(record.dwEventFlags)) { }

	ConsoleCoordinate Location;
	MouseButtonState ButtonState;
	int16_t Delta;
	ConsoleControlKeyStates ControlKeyState;
	MouseEventKind Kind;
};

struct BufferEventRecord
{
	constexpr BufferEventRecord() : Size() { }
	constexpr BufferEventRecord(const WINDOW_BUFFER_SIZE_RECORD& record) : Size(record.dwSize) { }

	ConsoleSize Size;
};

struct MenuEventRecord
{
	constexpr MenuEventRecord() : CommandId(0) { }
	constexpr MenuEventRecord(const MENU_EVENT_RECORD& record) : CommandId(record.dwCommandId) { }

	uint32_t CommandId;
};

struct FocusEventRecord
{
	constexpr FocusEventRecord() : IsSetFocus(false) { }
	constexpr FocusEventRecord(const FOCUS_EVENT_RECORD& record) : IsSetFocus(record.bSetFocus) { }

	bool IsSetFocus;
};

using EventRecord = std::variant<std::monostate, KeyEventRecord, MouseEventRecord, BufferEventRecord, MenuEventRecord, FocusEventRecord>;

constexpr inline EventRecord CreateEventRecord(const INPUT_RECORD& record)
{
	EventRecord eventRecord;
	switch (record.EventType)
	{
	case KEY_EVENT               : eventRecord.emplace<KeyEventRecord   >(record.Event.KeyEvent             ); break;
	case MOUSE_EVENT             : eventRecord.emplace<MouseEventRecord >(record.Event.MouseEvent           ); break;
	case WINDOW_BUFFER_SIZE_EVENT: eventRecord.emplace<BufferEventRecord>(record.Event.WindowBufferSizeEvent); break;
	case MENU_EVENT              : eventRecord.emplace<MenuEventRecord  >(record.Event.MenuEvent            ); break;
	case FOCUS_EVENT             : eventRecord.emplace<FocusEventRecord >(record.Event.FocusEvent           ); break;
	default: _ASSERT_EXPR(false, "Unreachable"); break;
	}
	return eventRecord;
}

class ConsoleBase abstract
{
public:
	ConsoleBase(HANDLE handle) : m_Handle(handle) { }
	ConsoleBase(DWORD kind) : m_Handle(GetHandle(kind)) { }
	ConsoleBase(const ConsoleBase&) = delete;
	ConsoleBase(ConsoleBase&&) = default;
	ConsoleBase& operator=(const ConsoleBase&) = delete;
	ConsoleBase& operator=(ConsoleBase&&) = default;

protected:
	HANDLE GetHandle() const { return m_Handle; }
	uint32_t GetModeCore() const
	{
		DWORD mode;
		ThrowIfFailed(GetConsoleMode(m_Handle, &mode));
		return mode;
	}
	void SetModeCore(uint32_t value) { ThrowIfFailed(SetConsoleMode(m_Handle, value)); }

private:
	HANDLE m_Handle;

	static HANDLE GetHandle(DWORD kind)
	{
		auto handle = GetStdHandle(kind);
		if (handle == INVALID_HANDLE_VALUE)
			ThrowLastException();
		return handle;
	}
};

class OutputConsole : public ConsoleBase
{
public:
	OutputConsole() : ConsoleBase(STD_OUTPUT_HANDLE) { }

	ConsoleOutputModes GetMode() const { return static_cast<ConsoleOutputModes>(GetModeCore()); }
	void SetMode(ConsoleOutputModes value) { return SetModeCore(static_cast<uint32_t>(value)); }
	ConsoleSize GetScreenBufferSize() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleSize(info.dwSize);
	}
	void SetScreenBufferSize(ConsoleSize value) { ThrowIfFailed(SetConsoleScreenBufferSize(GetHandle(), static_cast<COORD>(value))); }
	ConsoleCoordinate GetCursorPosition() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleCoordinate(info.dwCursorPosition);
	}
	void SetCursorPosition(ConsoleCoordinate position) { ThrowIfFailed(SetConsoleCursorPosition(GetHandle(), static_cast<COORD>(position))); }
	uint32_t GetCursorSize() const
	{
		CONSOLE_CURSOR_INFO info;
		ThrowIfFailed(GetConsoleCursorInfo(GetHandle(), &info));
		return info.dwSize;
	}
	void SetCursorSize(uint32_t value)
	{
		CONSOLE_CURSOR_INFO info;
		ThrowIfFailed(GetConsoleCursorInfo(GetHandle(), &info));
		info.dwSize = value;
		ThrowIfFailed(SetConsoleCursorInfo(GetHandle(), &info));
	}
	bool GetIsCursorVisible() const
	{
		CONSOLE_CURSOR_INFO info;
		ThrowIfFailed(GetConsoleCursorInfo(GetHandle(), &info));
		return info.bVisible;
	}
	void SetIsCursorVisible(bool value)
	{
		CONSOLE_CURSOR_INFO info;
		ThrowIfFailed(GetConsoleCursorInfo(GetHandle(), &info));
		info.bVisible = value;
		ThrowIfFailed(SetConsoleCursorInfo(GetHandle(), &info));
	}
	ConsoleRect GetWindowBounds() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleRect(info.srWindow);
	}
	void SetWindowBounds(bool absolute, const ConsoleRect& bounds)
	{
		auto rect = static_cast<SMALL_RECT>(bounds);
		ThrowIfFailed(SetConsoleWindowInfo(GetHandle(), absolute, &rect));
	}
	ConsoleSize GetMaximumWindowSize() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleSize(info.dwMaximumWindowSize);
	}
	ConsoleCharacterAttribute GetTextAttribute() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleCharacterAttribute(info.wAttributes);
	}
	void SetTextAttribute(ConsoleCharacterAttribute value) { ThrowIfFailed(SetConsoleTextAttribute(GetHandle(), static_cast<uint16_t>(value))); }
	ConsoleCharacterAttribute GetPopupAttribute() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return ConsoleCharacterAttribute(info.wPopupAttributes);
	}
	bool IsFullScreenSupported() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return info.bFullscreenSupported;
	}
	std::vector<COLORREF> GetColorTable() const
	{
		CONSOLE_SCREEN_BUFFER_INFOEX info;
		GetScreenBufferInfo(info);
		return std::vector<COLORREF>(info.ColorTable, info.ColorTable + sizeof(info.ColorTable) / sizeof(info.ColorTable[0]));
	}
	ConsoleFontInfo GetCurrentFont(bool maximumWindow) const
	{
		CONSOLE_FONT_INFOEX info{ sizeof(CONSOLE_FONT_INFOEX) };
		ThrowIfFailed(GetCurrentConsoleFontEx(GetHandle(), maximumWindow, &info));
		return ConsoleFontInfo(info);
	}
	void SetCurrentFont(bool maximumWindow, const ConsoleFontInfo& value)
	{
		CONSOLE_FONT_INFOEX info;
		value.CopyTo(info);
		ThrowIfFailed(SetCurrentConsoleFontEx(GetHandle(), maximumWindow, &info));
	}

	uint32_t FillOutput(WCHAR character, uint32_t length, ConsoleCoordinate coord)
	{
		DWORD actualLength;
		ThrowIfFailed(FillConsoleOutputCharacterW(GetHandle(), character, length, static_cast<COORD>(coord), &actualLength));
		return actualLength;
	}
	uint32_t FillOutput(ConsoleCharacterAttribute attribute, uint32_t length, ConsoleCoordinate coord)
	{
		DWORD actualLength;
		ThrowIfFailed(FillConsoleOutputAttribute(GetHandle(), static_cast<uint16_t>(attribute), length, static_cast<COORD>(coord), &actualLength));
		return actualLength;
	}
	uint32_t WriteOutput(std::wstring_view characters, ConsoleCoordinate coord)
	{
		DWORD actualLength;
		ThrowIfFailed(WriteConsoleOutputCharacterW(GetHandle(), characters.data(), static_cast<DWORD>(characters.size()), static_cast<COORD>(coord), &actualLength));
		return actualLength;
	}
	template <typename InputIterator> uint32_t WriteOutput(InputIterator attributesBegin, InputIterator attributesEnd, ConsoleCoordinate coord)
	{
		std::vector<uint16_t> buffer;
		std::transform(attributesBegin, attributesEnd, std::back_insert_iterator(buffer), [](const ConsoleCharacterAttribute& attr) { return static_cast<uint16_t>(attr); });
		DWORD actualLength;
		ThrowIfFailed(WriteConsoleOutputAttribute(m_Handle, buffer.data(), static_cast<DWORD>(buffer.size()), static_cast<COORD>(coord), &actualLength));
		return actualLength;
	}
	uint32_t Write(std::wstring_view text)
	{
		DWORD actualLength;
		ThrowIfFailed(WriteConsoleW(GetHandle(), text.data(), static_cast<DWORD>(text.size()), &actualLength, nullptr));
		return actualLength;
	}

private:
	void GetScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFOEX& value) const
	{
		value.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
		ThrowIfFailed(GetConsoleScreenBufferInfoEx(GetHandle(), &value));
	}
};

class InputConsole : public ConsoleBase
{
public:
	InputConsole() : ConsoleBase(STD_INPUT_HANDLE) { }

	ConsoleInputModes GetMode() const { return static_cast<ConsoleInputModes>(GetModeCore()); }
	void SetMode(ConsoleInputModes value) { return SetModeCore(static_cast<uint32_t>(value)); }
	uint32_t GetNumberOfInputEvents() const
	{
		DWORD numberOfEvents;
		ThrowIfFailed(GetNumberOfConsoleInputEvents(GetHandle(), &numberOfEvents));
		return numberOfEvents;
	}
	uint32_t PeekInput(INPUT_RECORD* buffer, uint32_t length) const { return PeekReadInput(PeekConsoleInputW, GetHandle(), buffer, length); }
	std::optional<EventRecord> PeekInput() const
	{
		INPUT_RECORD inputRecord;
		std::optional<EventRecord> value;
		if (PeekReadInput(PeekConsoleInputW, GetHandle(), &inputRecord, 1) != 0)
			value = CreateEventRecord(inputRecord);
		return value;
	}
	std::vector<EventRecord> PeekInput(uint32_t length) const { return PeekReadInput(PeekConsoleInputW, GetHandle(), length); }
	uint32_t ReadInput(INPUT_RECORD* buffer, uint32_t length) { return PeekReadInput(ReadConsoleInputW, GetHandle(), buffer, length); }
	EventRecord ReadInput()
	{
		INPUT_RECORD inputRecord;
		PeekReadInput(ReadConsoleInputW, GetHandle(), &inputRecord, 1);
		return CreateEventRecord(inputRecord);
	}
	std::vector<EventRecord> ReadInput(uint32_t length) { return PeekReadInput(ReadConsoleInputW, GetHandle(), length); }
	uint32_t Read(WCHAR* buffer, uint32_t length, const std::optional<CONSOLE_READCONSOLE_CONTROL>& control = std::nullopt)
	{
		DWORD actualLength;
		CONSOLE_READCONSOLE_CONTROL* pControl = nullptr;
		if (control)
			pControl = const_cast<CONSOLE_READCONSOLE_CONTROL*>(&*control);
		ThrowIfFailed(ReadConsoleW(GetHandle(), buffer, length, &actualLength, pControl));
		return actualLength;
	}
	std::wstring Read(const std::optional<CONSOLE_READCONSOLE_CONTROL>& control = std::nullopt)
	{
		constexpr uint32_t charsToRead = 16;
		std::wstring buffer;
		size_t charsReadSoFar = 0;
		while (true)
		{
			buffer.resize(charsReadSoFar + charsToRead);
			const auto charsRead = Read(buffer.data() + charsReadSoFar, charsToRead, control);
			charsReadSoFar += charsRead;
			if (charsRead < charsToRead) break;
		}
		buffer.resize(charsReadSoFar);
		buffer.shrink_to_fit();
		return std::move(buffer);
	}
	void FlushInputBuffer() { ThrowIfFailed(FlushConsoleInputBuffer(GetHandle())); }

	static uint32_t GetNumberOfMouseButtons()
	{
		DWORD numberOfMouseButtons;
		ThrowIfFailed(GetNumberOfConsoleMouseButtons(&numberOfMouseButtons));
		return numberOfMouseButtons;
	}

private:
	using PeekReadFunc = BOOL(WINAPI *)(HANDLE, PINPUT_RECORD, DWORD, LPDWORD);

	static uint32_t PeekReadInput(PeekReadFunc func, HANDLE handle, INPUT_RECORD* buffer, uint32_t length)
	{
		DWORD actualLength;
		ThrowIfFailed(func(handle, buffer, length, &actualLength));
		return actualLength;
	}
	static std::vector<EventRecord> PeekReadInput(PeekReadFunc func, HANDLE handle, uint32_t length)
	{
		std::vector<INPUT_RECORD> buffer(length);
		auto actualLength = PeekReadInput(func, handle, buffer.data(), length);
		std::vector<EventRecord> records(actualLength);
		std::transform(buffer.cbegin(), buffer.cbegin() + actualLength, records.begin(), CreateEventRecord);
		return records;
	}
};
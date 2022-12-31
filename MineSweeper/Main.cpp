#include <array>
#include <iostream>
#include <queue>
#include <string>
#include <functional>
#include <random>
#include <ranges>
#include "Console.h"

constexpr ConsoleColor DefaultBackground = ConsoleColor::Silver;
constexpr ConsoleColor DefaultForeground = ConsoleColor::Black;

struct Vector
{
	constexpr Vector() : X(0), Y(0) {}
	constexpr Vector(int32_t x, int32_t y) : X(x), Y(y) { }

	int32_t X;
	int32_t Y;

	constexpr bool operator ==(const Vector& right) const { return X == right.X && Y == right.Y; }
	constexpr bool operator !=(const Vector& right) const { return !(*this == right); }
	constexpr Vector& operator +=(const Vector& right)
	{
		X += right.X;
		Y += right.Y;
		return *this;
	}
	constexpr Vector operator +(const Vector& right) const { return Vector(*this) += right; }
	constexpr Vector& operator -=(const Vector& right)
	{
		X -= right.X;
		Y -= right.Y;
		return *this;
	}
	constexpr Vector operator -(const Vector& right) const { return Vector(*this) -= right; }
	constexpr Vector operator +() const { return *this; }
	constexpr Vector operator -() const { return { -X, -Y }; }
};

struct Size
{
	constexpr Size() : Width(0), Height(0) {}
	constexpr Size(uint32_t width, uint32_t height) : Width(width), Height(height) {}
	explicit Size(const Vector& vector) : Width(static_cast<uint32_t>(abs(vector.X))), Height(static_cast<uint32_t>(abs(vector.Y))) {}
	
	uint32_t Width;
	uint32_t Height;

	constexpr bool operator ==(const Size& right) const { return Width == right.Width && Height == right.Height; }
	constexpr bool operator !=(const Size& right) const { return !(*this == right); }
	constexpr explicit operator Vector() const { return Vector(static_cast<int32_t>(Width), static_cast<int32_t>(Height)); }
};

struct Point
{
public:
	constexpr Point() : X(0), Y(0) { }
	constexpr Point(uint32_t x, uint32_t y) : X(x), Y(y) { }

	uint32_t X;
	uint32_t Y;

	constexpr Point& operator +=(const Vector& right)
	{
		X += right.X;
		Y += right.Y;
		return *this;
	}
	constexpr Point operator +(const Vector& right) const { return Point(*this) += right; }
	constexpr Point& operator -=(const Vector& right)
	{
		X -= right.X;
		Y -= right.Y;
		return *this;
	}
	constexpr Point operator -(const Vector& right) const { return Point(*this) -= right; }
	constexpr Vector operator -(const Point& right) const { return Vector(SafeSubtract(X, right.X), SafeSubtract(Y, right.Y)); }
	constexpr bool operator ==(const Point& right) const { return X == right.X && Y == right.Y; }
	constexpr bool operator !=(const Point& right) const { return !(*this == right); }

	constexpr bool IsContainedIn(const Size& size) const { return X < size.Width&& Y < size.Height; }

private:
	constexpr static int32_t SafeSubtract(uint32_t left, uint32_t right)
	{
		return left >= right ?
			static_cast<int32_t>(left - right) :
			-static_cast<int32_t>(right - left);
	}
};

constexpr inline Point operator +(const Vector& left, const Point& right) { return right + left; }

enum class GameProgress
{
	InProgress = 0,
	Completed = 1,
	Failed = 2,
};

class AllPointView : public std::ranges::view_interface<AllPointView>
{
public:
	class Sentinel {};
	class Iterator
	{
	public:
		constexpr Iterator(const Size& size) : m_Size(size), m_Value() {}
		constexpr Iterator& operator ++() { m_Value = m_Value.X >= m_Size.Width - 1 ? Point(0, m_Value.Y + 1) : Point(m_Value.X + 1, m_Value.Y); return *this; }
		constexpr void operator ++(int) { operator ++(); }
		constexpr const Point& operator *() const { return m_Value; }
		constexpr bool operator ==(Sentinel) const { return !m_Value.IsContainedIn(m_Size); }

		using difference_type = ptrdiff_t;
		using value_type = Point;
	private:
		Point m_Value;
		Size m_Size;
	};

	constexpr AllPointView(const Size& size) : m_Size(size) {}
	constexpr Iterator begin() const { return Iterator(m_Size); }
	constexpr Sentinel end() const { return {}; }

private:
	Size m_Size;
};

template <>
static inline constexpr bool std::ranges::enable_borrowed_range<AllPointView> = true;

class AroundPointView : public std::ranges::view_interface<AroundPointView>
{
public:
	class Sentinel {};
	class Iterator
	{
	public:
		constexpr Iterator(const Point& center, const Size& size) : m_Center(center), m_Size(size), m_Index(static_cast<uint32_t>(-1)) { operator ++(); }
		constexpr Iterator& operator ++()
		{
			do
				m_Index++;
			while (m_Index < End && (m_Index == Skip || !operator *().IsContainedIn(m_Size)));
			return *this;
		}
		constexpr void operator ++(int) { operator ++(); }
		constexpr Point operator *() const { return m_Center + Vector(m_Index % 3 - 1, m_Index / 3 - 1); }
		constexpr bool operator ==(Sentinel) const { return m_Index == End; }

		using difference_type = int32_t;
		using value_type = Point;

	private:
		constexpr static uint32_t Skip = 4;
		constexpr static uint32_t End = 9;
		Point m_Center;
		Size m_Size;
		uint32_t m_Index;
	};

	constexpr AroundPointView(const Point& center, const Size& size) : m_Center(center), m_Size(size) {}
	constexpr Iterator begin() const { return Iterator(m_Center, m_Size); }
	constexpr Sentinel end() const { return {}; }

private:
	Point m_Center;
	Size m_Size;
};

template <>
static inline constexpr bool std::ranges::enable_borrowed_range<AroundPointView> = true;

class Game
{
public:
	Game(const Size& size, uint32_t mines) : m_Cells(std::make_unique<Cell[]>(static_cast<size_t>(size.Width) * size.Height)), m_Size(size), m_MinesToBePlaced(mines), m_ShouldRender(true) { }

	std::optional<Point> CoordinateToLocation(ConsoleCoordinate coordinate) const
	{
		Point loc(static_cast<uint32_t>(coordinate.X / 2), static_cast<uint32_t>(coordinate.Y));
		if (loc.IsContainedIn(m_Size))
			return { loc };
		else
			return std::nullopt;
	}
	constexpr bool ShouldRender() const { return m_ShouldRender; }
	void Render(OutputConsole& output)
	{
		output.SetCursorPosition({ 0, 0 });
		for (uint32_t i = 0; i < m_Size.Height; i++)
		{
			for (uint32_t j = 0; j < m_Size.Width; j++)
				CellAt(j, i).Render(output, m_OpeningPosition && IsAround(Point(j, i), *m_OpeningPosition));
			output.Write(L"\n");
		}
		output.FillOutput(L' ', output.GetScreenBufferSize().Width, output.GetCursorPosition());
		output.Write(L"残り地雷数: " + std::to_wstring(CountUnflaggedMines()));
		m_ShouldRender = false;
	}
	void OpenCell(const Point& loc)
	{
		std::queue<Point> searchLocations;
		searchLocations.emplace(loc);
		while (!searchLocations.empty())
		{
			Point loc = searchLocations.front();
			searchLocations.pop();
			if (CellAt(loc).State == CellState::Flagged || CellAt(loc).State == CellState::Open)
				continue;
			if (m_MinesToBePlaced > 0)
			{
				PlaceMines(m_MinesToBePlaced, loc);
				m_MinesToBePlaced = 0;
			}
			CellAt(loc).State = CellState::Open;
			m_ShouldRender = true;
			if (CellAt(loc).HasMine)
			{
				OpenAllMines();
				continue;
			}
			if (CellAt(loc).AroundMines > 0)
				continue;
			for (auto pos : AroundPointView(loc, m_Size))
				searchLocations.emplace(pos);
		}
	}
	constexpr void OpenCellsWithMineIndicator(const Point& loc)
	{
		if (CellAt(loc).State != CellState::Open)
			return;
		size_t allArounds = 0;
		std::vector<Point> locs;
		for (auto pos : AroundPointView(loc, m_Size))
		{
			if (CellAt(pos).State != CellState::Flagged)
				locs.emplace_back(pos);
			allArounds++;
		}
		if (locs.size() != allArounds - CellAt(loc).AroundMines)
			return;
		for (const auto& it : locs)
			OpenCell(it);
	}
	constexpr void SwitchFlaggedState(const Point& loc) { m_ShouldRender |= CellAt(loc).SwitchFlaggedState(); }
	constexpr void SetCellOpening(const Point& loc)
	{
		ClearCellOpening();
		m_OpeningPosition = loc;
		m_ShouldRender |= true;
	}
	constexpr void ClearCellOpening()
	{
		m_OpeningPosition = std::nullopt;
		m_ShouldRender |= true;
	}
	constexpr bool IsOpeningAnyCell() const { return m_OpeningPosition.has_value(); }
	constexpr GameProgress GetProgress() const
	{
		GameProgress result = GameProgress::Completed;
		for (const auto& cell : Cells())
		{
			// 地雷があるが開かれていた（地雷がある場合は即時returnする）
			if (cell.HasMine && cell.State == CellState::Open)
				return GameProgress::Failed;
			// 地雷がないのに開かれていない（以降のセルで地雷が開かれている可能性があるため即時returnはしない）
			if (!cell.HasMine && cell.State != CellState::Open)
				result = GameProgress::InProgress;
			// 下記は完了の可能性があるので判定を継続する
			// * 地雷があって開かれていない
			// * 地雷がなくて開かれている
		}
		return result;
	}

private:
	enum class CellState : uint8_t
	{
		Closed = 0,
		Flagged = 1,
		Open = 2,
	};

	class Cell
	{
	public:
		constexpr Cell() : AroundMines(0), HasMine(false), State(CellState::Closed) { }
		uint8_t AroundMines : 5;
		uint8_t HasMine : 1;
		CellState State : 2;

		void Render(OutputConsole& output, bool opening) const
		{
			if (State == CellState::Flagged)
			{
				output.SetTextAttribute({ ConsoleColor::Purple, DefaultBackground });
				output.Write(L"■");
				output.SetTextAttribute({ DefaultForeground, DefaultBackground });
			}
			else if (State != CellState::Open)
			{
				output.SetTextAttribute({ opening ? ConsoleColor::Black : ConsoleColor::Gray, DefaultBackground });
				output.Write(L"■");
				output.SetTextAttribute({ DefaultForeground, DefaultBackground });
			}
			else if (HasMine)
				output.Write(L"●");
			else if (AroundMines == 0)
				output.Write(L"  ");
			else
			{
				output.SetTextAttribute({ GetColor(AroundMines), DefaultBackground });
				auto ch = static_cast<WCHAR>(L'０' + AroundMines);
				output.Write(std::wstring_view(&ch, 1));
				output.SetTextAttribute({ DefaultForeground, DefaultBackground });
			}
		}
		constexpr bool SwitchFlaggedState()
		{
			if (State == CellState::Closed)
			{
				State = CellState::Flagged;
				return true;
			}
			else if (State == CellState::Flagged)
			{
				State = CellState::Closed;
				return true;
			}
			return false;
		}

	private:
		constexpr static ConsoleColor GetColor(int value)
		{
			switch (value)
			{
			case 1:  return ConsoleColor::Blue;
			case 2:  return ConsoleColor::Green;
			case 4:  return ConsoleColor::Navy;
			case 5:  return ConsoleColor::Maroon;
			case 6:  return ConsoleColor::Teal;
			default: return ConsoleColor::Red;
			}
		}
	};

	constexpr Cell& CellAt(uint32_t x, uint32_t y) { return m_Cells[static_cast<size_t>(y) * m_Size.Width + x]; }
	constexpr const Cell& CellAt(uint32_t x, uint32_t y) const { return m_Cells[static_cast<size_t>(y) * m_Size.Width + x]; }
	constexpr Cell& CellAt(const Point& loc) { return CellAt(loc.X, loc.Y); }
	constexpr const Cell& CellAt(const Point& loc) const { return CellAt(loc.X, loc.Y); }
	constexpr std::span<Cell> Cells() { return std::span<Cell>(m_Cells.get(), static_cast<size_t>(m_Size.Width) * m_Size.Height); }
	constexpr std::span<const Cell> Cells() const { return std::span<Cell>(m_Cells.get(), static_cast<size_t>(m_Size.Width) * m_Size.Height); }

	void PlaceMines(uint32_t mines, const Point& without)
	{
		std::array<std::seed_seq::result_type, std::mt19937::state_size> seed_data{};
		std::random_device rnd;
		std::generate(seed_data.begin(), seed_data.end(), std::ref(rnd));
		std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
		std::mt19937 rng(seq);
		for (uint32_t i = 0; i < mines; )
		{
			auto loc = GenerateLocation(rng);
			bool matches = false;
			if (mines <= m_Size.Width * m_Size.Height - 9)
				matches |= IsAround(loc, without);
			if (without == loc || matches || CellAt(loc).HasMine)
				continue;
			CellAt(loc).HasMine = true;
			i++;
		}
		for (const auto& loc : AllPointView(m_Size))
			CellAt(loc).AroundMines = std::ranges::count_if(AroundPointView(loc, m_Size), [this](auto x) { return CellAt(x).HasMine; });
	}
	constexpr void OpenAllMines()
	{
		for (auto& cell : Cells())
		{
			if (cell.HasMine)
				cell.State = CellState::Open;
		}
	}
	constexpr int32_t CountUnflaggedMines() const
	{
		int32_t mines = 0;
		int32_t flags = 0;
		for (const auto& cell : Cells())
		{
			if (cell.HasMine)
				mines++;
			if (cell.State == CellState::Flagged)
				flags++;
		}
		return m_MinesToBePlaced + mines - flags;
	}

	template <typename TEngine> Point GenerateLocation(TEngine& engine) { return Point(std::uniform_int<uint32_t>(0, m_Size.Width - 1)(engine), std::uniform_int<uint32_t>(0, m_Size.Height - 1)(engine)); }

	constexpr bool IsAround(const Point& loc, const Point& center) { return std::ranges::contains(AroundPointView(center, m_Size), loc); }

	std::unique_ptr<Cell[]> m_Cells;
	Size m_Size;
	uint32_t m_MinesToBePlaced;
	std::optional<Point> m_OpeningPosition;
	bool m_ShouldRender;
};

bool PlayGame(const Size& size, uint32_t mines, InputConsole& input, OutputConsole& output)
{
	Game game(size, mines);
	std::optional<MouseButtonState> prevButtonState;
	while (true)
	{
		if (game.ShouldRender())
		{
			game.Render(output);
			switch (game.GetProgress())
			{
			case GameProgress::Failed   : return false;
			case GameProgress::Completed: return true;
			}
		}
		const auto eventRecord = input.ReadInput();
		const auto ev = std::get_if<MouseEventRecord>(&eventRecord);
		if (!ev) continue;
		auto loc = game.CoordinateToLocation(ev->Location);
		if (prevButtonState && loc)
		{
			// 左右両ボタン押下→少なくとも左右いずれのボタンが非押下
			if (prevButtonState->GetLeft() && prevButtonState->GetRight() && (!ev->ButtonState.GetLeft() || !ev->ButtonState.GetRight()))
			{
				game.ClearCellOpening();
				game.OpenCellsWithMineIndicator(*loc);
			}
			// 左ボタンのみ押下→左右両ボタン非押下
			if (prevButtonState->GetLeft() && !prevButtonState->GetRight() && !ev->ButtonState.GetLeft() && !ev->ButtonState.GetRight())
				game.OpenCell(*loc);
			// 右ボタンのみ押下→左右両ボタン非押下
			if (!prevButtonState->GetLeft() && prevButtonState->GetRight() && !ev->ButtonState.GetLeft() && !ev->ButtonState.GetRight())
				game.SwitchFlaggedState(*loc);
			// 少なくとも左右いずれかのボタンが非押下→左右両ボタン押下
			if ((!prevButtonState->GetLeft() || !prevButtonState->GetRight()) && ev->ButtonState.GetLeft() && ev->ButtonState.GetRight())
				game.SetCellOpening(*loc);
			if (game.IsOpeningAnyCell() && ev->Kind == MouseEventKind::Moved)
				game.SetCellOpening(*loc);
		}
		prevButtonState = ev->ButtonState;
	}
}

long InputLongValue(InputConsole& input, OutputConsole& output, std::wstring_view valueName, long minValue, long maxValue)
{
	auto initialAttribute = output.GetTextAttribute();
	long value;
	while (true)
	{
		output.Write(valueName);
		output.Write(L": ");
		try
		{
			value = std::stol(input.Read());
		}
		catch (...)
		{
			output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
			output.Write(valueName);
			output.Write(L"を数値で入力してください。\n");
			output.SetTextAttribute(initialAttribute);
			continue;
		}
		if (value < minValue)
		{
			output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
			output.Write(valueName);
			output.Write(L"は");
			output.Write(std::to_wstring(minValue));
			output.Write(L"以上の値を入力してください。\n");
			output.SetTextAttribute(initialAttribute);
			continue;
		}
		if (value > maxValue)
		{
			output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
			output.Write(valueName);
			output.Write(L"は");
			output.Write(std::to_wstring(maxValue));
			output.Write(L"以下の値を入力してください。\n");
			output.SetTextAttribute(initialAttribute);
			continue;
		}
		return value;
	}

}

int main()
{
	InputConsole input;
	OutputConsole output;
	const auto initialAttribute = output.GetTextAttribute();
	input.SetMode((input.GetMode() & ~ConsoleInputModes::EnableQuickEditMode) | ConsoleInputModes::EnableMouseInput);

	bool enterConfiguration = true;
	Size size;
	long mines;
	while (true)
	{
		if (enterConfiguration)
		{
			size.Width = InputLongValue(input, output, L"幅", 1, 60);
			size.Height = InputLongValue(input, output, L"高さ", 1, 40);
			mines = InputLongValue(input, output, L"地雷数", 0, size.Width * size.Height - 1);
		}

		output.SetCursorPosition({ 0, 0 });
		const auto bufferSize = output.GetScreenBufferSize();
		output.FillOutput(L' ', bufferSize.Width * bufferSize.Height, { 0, 0 });
		output.FillOutput({ DefaultForeground, DefaultBackground }, bufferSize.Width * bufferSize.Height, { 0, 0 });
		output.SetTextAttribute({ DefaultForeground, DefaultBackground });
		const auto intialWindowBounds = output.GetWindowBounds();
		const auto initialFontInfo = output.GetCurrentFont(false);
		auto newFontInfo = initialFontInfo;
		newFontInfo.Size.Width = 20;
		newFontInfo.Size.Height = 40;
		output.SetCurrentFont(false, newFontInfo);
		output.SetWindowBounds(true, { 0, 0, static_cast<int16_t>(size.Width * 2 - 1), static_cast<int16_t>(size.Height + 1 - 1) });

		bool result = PlayGame(size, mines, input, output);

		output.SetCurrentFont(false, initialFontInfo);
		output.SetWindowBounds(true, intialWindowBounds);
		output.Write(L"\n");
		auto pos = output.GetCursorPosition();
		pos.X = 0;
		output.FillOutput(initialAttribute, (bufferSize.Height - pos.Y) * bufferSize.Width, pos);

		if (result)
		{
			output.SetTextAttribute({ ConsoleColor::Lime, initialAttribute.Background });
			output.Write(L"おめでとうございます！\n");
			output.Write(L"すべての地雷を取り除きました！\n");
		}
		else
		{
			output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
			output.Write(L"地雷を踏んでしまいました...\n");
		}
		output.SetTextAttribute(initialAttribute);

		output.Write(L"もう一度プレイする場合は [R] を、設定を変更してプレイする場合は [Shift] + [R] を、終了する場合は [Q] を押してください\n");
		while (true)
		{
			auto eventRecord = input.ReadInput();
			auto ev = std::get_if<KeyEventRecord>(&eventRecord);
			if (!ev) continue;
			if (ev->Char == 'r')
			{
				enterConfiguration = false;
				break;
			}
			if (ev->Char == 'R')
			{
				enterConfiguration = true;
				break;
			}
			if (ev->Char == 'q') goto Exit;
		}
	}

Exit:;
}

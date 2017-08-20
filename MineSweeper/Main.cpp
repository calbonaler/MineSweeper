#include <iostream>
#include <string>
#include <functional>
#include <random>
#include "Console.h"

constexpr ConsoleColor DefaultBackground = ConsoleColor::Silver;
constexpr ConsoleColor DefaultForeground = ConsoleColor::Black;

struct Vector
{
	Vector() : X(0), Y(0) {}
	Vector(ptrdiff_t x, ptrdiff_t y) : X(x), Y(y) { }

	ptrdiff_t X;
	ptrdiff_t Y;

	bool operator ==(const Vector& right) const { return X == right.X && Y == right.Y; }
	bool operator !=(const Vector& right) const { return !(*this == right); }
};

struct Location
{
public:
	Location() : X(0), Y(0) { }
	Location(size_t x, size_t y) : X(x), Y(y) { }

	size_t X;
	size_t Y;

	Location& operator +=(const Vector& right)
	{
		X += right.X;
		Y += right.Y;
		return *this;
	}
	Location operator +(const Vector& right) const { return Location(*this) += right; }
	Location& operator -=(const Vector& right)
	{
		X -= right.X;
		Y -= right.Y;
		return *this;
	}
	Location operator -(const Vector& right) const { return Location(*this) -= right; }
	Vector operator -(const Location& right) const { return Vector(SafeSubtract(X, right.X), SafeSubtract(Y, right.Y)); }
	bool operator ==(const Location& right) const { return X == right.X && Y == right.Y; }
	bool operator !=(const Location& right) const { return !(*this == right); }

private:
	static inline ptrdiff_t SafeSubtract(size_t left, size_t right)
	{
		return left >= right ?
			static_cast<ptrdiff_t>(left - right) :
			-static_cast<ptrdiff_t>(right - left);
	}
};

inline Location operator +(const Vector& left, const Location& right) { return right + left; }

class Game
{
public:
	Game(size_t width, size_t height, size_t mines) : table(width, height), mines(mines), minesInitialized(false) { }
	Game(const Game&) = delete;
	Game(Game&&) = delete;
	Game& operator =(const Game&) = delete;
	Game& operator =(Game&&) = delete;

	static Location GetLocationForCoordinate(ConsoleCoordinate coordinate) { return { static_cast<size_t>(coordinate.X / 2), static_cast<size_t>(coordinate.Y) }; }

	void Render(OutputConsole& output) const
	{
		for (size_t y = 0; y < table.Height(); y++)
		{
			for (size_t x = 0; x < table.Width(); x++)
				table(x, y).Render(output);
			output.Write(L"\n");
		}
	}
	bool OpenCell(const Location& loc)
	{
		if (table(loc).State == CellState::Flagged || table(loc).State == CellState::Open)
			return true;
		if (!minesInitialized)
		{
			SetMines(loc);
			minesInitialized = true;
		}
		table(loc).State = CellState::Open;
		if (table(loc).HasMine)
		{
			OpenAllMines();
			return false;
		}
		if (table(loc).AroundMines > 0)
			return true;
		table.Around(loc, [this](const Location& loc) { OpenCell(loc); });
		return true;
	}
	bool OpenCellsWithCurrentMineIndicator(const Location& loc)
	{
		if (table(loc).State != CellState::Open)
			return true;
		size_t allArounds = 0;
		std::vector<Location> locs;
		table.Around(loc, [this, &allArounds, &locs](const Location& loc)
		{
			if (table(loc).State != CellState::Flagged)
				locs.emplace_back(loc);
			allArounds++;
		});
		if (locs.size() != allArounds - table(loc).AroundMines)
			return true;
		for (const auto& it : locs)
		{
			if (!OpenCell(it))
				return false;
		}
		return true;
	}
	void SwitchFlaggedState(const Location& loc)
	{
		if (table(loc).State == CellState::Closed)
			table(loc).State = CellState::Flagged;
		else if (table(loc).State == CellState::Flagged)
			table(loc).State = CellState::Closed;
	}
	void SetCellOpeningState(const Location& loc, bool opening)
	{
		table.Around(loc, [this, opening](const Location& loc)
		{
			if (opening)
			{
				if (table(loc).State == CellState::Closed)
					table(loc).State = CellState::Opening;
			}
			else
			{
				if (table(loc).State == CellState::Opening)
					table(loc).State = CellState::Closed;
			}
		});
	}
	bool IsValidLocation(const Location& loc) const { return table.IsValidLocation(loc); }
	bool HasCompleted() const
	{
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (!table(loc).HasMine && table(loc).State != CellState::Open)
				return false;
		}
		return true;
	}
	ptrdiff_t CountUnflaggedMines() const
	{
		ptrdiff_t allMines = static_cast<ptrdiff_t>(mines);
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (table(loc).State == CellState::Flagged)
				--allMines;
		}
		return allMines;
	}

private:
	enum class CellState : uint8_t
	{
		Closed = 0,
		Flagged = 1,
		Opening = 2,
		Open = 3,
	};

	class Cell
	{
	public:
		Cell() : AroundMines(0), HasMine(false), State(CellState::Closed) { }
		uint8_t AroundMines : 5;
		uint8_t HasMine : 1;
		CellState State : 2;

		void Render(OutputConsole& output) const
		{
			if (State == CellState::Flagged)
			{
				output.SetTextAttribute({ ConsoleColor::Red, DefaultBackground });
				output.Write(L"★");
				output.SetTextAttribute({ DefaultForeground, DefaultBackground });
			}
			else if (State != CellState::Open)
			{
				output.SetTextAttribute({ State == CellState::Opening ? ConsoleColor::Black : ConsoleColor::Gray, DefaultBackground });
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
				output.Write(&ch, 1);
				output.SetTextAttribute({ DefaultForeground, DefaultBackground });
			}
		}
		static ConsoleColor GetColor(int value)
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

	class Table
	{
	public:
		Table(size_t width, size_t height) : cells(height, std::vector<Cell>(width)) { }
		Table(const Table&) = delete;
		Table(Table&&) = delete;
		Table& operator =(const Table&) = delete;
		Table& operator =(Table&&) = delete;

		Cell& operator()(size_t x, size_t y) { return cells[y][x]; }
		const Cell& operator()(size_t x, size_t y) const { return cells[y][x]; }
		Cell& operator()(const Location& loc) { return (*this)(loc.X, loc.Y); }
		const Cell& operator()(const Location& loc) const { return (*this)(loc.X, loc.Y); }

		size_t Width() const { return cells[0].size(); }
		size_t Height() const { return cells.size(); }

		Location GetNextLocation(const Location& loc) const { return loc.X >= cells[0].size() - 1 ? Location(0, loc.Y + 1) : Location(loc.X + 1, loc.Y); }
		bool IsValidLocation(const Location& loc) const { return loc.X < cells[0].size() && loc.Y < cells.size(); }
		template <typename T> void Around(const Location& loc, T func) const
		{
			for (ptrdiff_t dx = -1; dx <= 1; dx++)
			{
				for (ptrdiff_t dy = -1; dy <= 1; dy++)
				{
					if (dx == 0 && dy == 0)
						continue;
					auto moved = loc + Vector(dx, dy);
					if (!IsValidLocation(moved))
						continue;
					func(moved);
				}
			}
		}
		template <typename TEngine> Location GenerateLocation(TEngine& engine) { return Location(std::uniform_int<size_t>(0, cells[0].size() - 1)(engine), std::uniform_int<size_t>(0, cells.size() - 1)(engine)); }

	private:
		std::vector<std::vector<Cell>> cells;
	};

	Table table;
	size_t mines;
	bool minesInitialized;

	void SetMines(const Location& without)
	{
		std::array<std::seed_seq::result_type, std::mt19937::state_size> seed_data;
		std::random_device rnd;
		std::generate(seed_data.begin(), seed_data.end(), std::ref(rnd));
		std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
		std::mt19937 rng(seq);
		for (size_t i = 0; i < mines; )
		{
			auto loc = table.GenerateLocation(rng);
			if (without == loc || table(loc).HasMine)
				continue;
			table(loc).HasMine = true;
			i++;
		}
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
			table(loc).AroundMines = CountMines(loc);
	}
	int CountMines(const Location& center) const
	{
		int mines = 0;
		table.Around(center, [this, &mines](const Location& moved) { if (table(moved).HasMine) mines++; });
		return mines;
	}
	void OpenAllMines()
	{
		for (Location loc; table.IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (table(loc).HasMine)
				table(loc).State = CellState::Open;
		}
	}
};

bool PlayGame(size_t width, size_t height, size_t mines, InputConsole& input, OutputConsole& output, ConsoleSize bufferSize)
{
	Game game(width, height, mines);
	std::optional<MouseButtonState> prevButtonState;
	std::optional<Location> openingCenterPos;
	bool renderRequested = true;
	bool canOpenCell = false;
	bool res = true;
	while (true)
	{
		if (renderRequested)
		{
			output.SetCursorPosition({ 0, 0 });
			game.Render(output);
			output.FillOutput(L' ', bufferSize.Width, output.GetCursorPosition());
			output.Write(L"残り地雷数: " + std::to_wstring(game.CountUnflaggedMines()));
			if (!res)
				return false;
			if (game.HasCompleted())
				return true;
			renderRequested = false;
		}
		const auto ev = input.ReadInput().AsMouseEvent();
		if (!ev) continue;
		auto loc = Game::GetLocationForCoordinate(ev->Location);
		if (prevButtonState && game.IsValidLocation(loc))
		{
			if (prevButtonState->GetLeft() && prevButtonState->GetRight() && (!ev->ButtonState.GetLeft() || !ev->ButtonState.GetRight()))
			{
				if (openingCenterPos)
				{
					game.SetCellOpeningState(*openingCenterPos, false);
					openingCenterPos = std::nullopt;
				}
				res = game.OpenCellsWithCurrentMineIndicator(loc);
				renderRequested = true;
				canOpenCell = false;
			}
			if (canOpenCell && prevButtonState->GetLeft() && !prevButtonState->GetRight() && !ev->ButtonState.GetLeft() && !ev->ButtonState.GetRight())
			{
				res = game.OpenCell(loc);
				renderRequested = true;
				canOpenCell = false;
			}
			if (!prevButtonState->GetLeft() && !prevButtonState->GetRight() && !ev->ButtonState.GetLeft() && ev->ButtonState.GetRight())
			{
				game.SwitchFlaggedState(loc);
				res = true;
				renderRequested = true;
			}
			if (!prevButtonState->GetLeft() && !prevButtonState->GetRight() && ev->ButtonState.GetLeft() && !ev->ButtonState.GetRight())
				canOpenCell = true;
			if ((!prevButtonState->GetLeft() || !prevButtonState->GetRight()) && ev->ButtonState.GetLeft() && ev->ButtonState.GetRight())
			{
				openingCenterPos = loc;
				game.SetCellOpeningState(loc, true);
				renderRequested = true;
			}
			if (ev->Kind == MouseEventKind::Moved && openingCenterPos && *openingCenterPos != loc)
			{
				game.SetCellOpeningState(*openingCenterPos, false);
				openingCenterPos = loc;
				game.SetCellOpeningState(loc, true);
				renderRequested = true;
			}
		}
		prevButtonState = ev->ButtonState;
	}
}

int main()
{
	InputConsole input;
	OutputConsole output;
	const auto initialAttribute = output.GetTextAttribute();
	input.SetMode(input.GetMode() | ConsoleInputModes::EnableMouseInput);

	bool enterConfiguration = true;
	unsigned long width, height, mines;
	while (true)
	{
		if (enterConfiguration)
		{
			output.Write(L"幅: ");
			width = std::stoul(input.Read());
			output.Write(L"高さ: ");
			height = std::stoul(input.Read());
			while (true)
			{
				output.Write(L"地雷数: ");
				mines = std::stoul(input.Read());
				if (mines < width * height) break;
				output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
				output.Write(L"地雷数は幅×高さよりも小さくなければなりません。\n");
				output.SetTextAttribute(initialAttribute);
			}
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
		output.SetWindowBounds(true, { 0, 0, static_cast<int16_t>(width * 2 - 1), static_cast<int16_t>(height + 1 - 1) });

		bool result = PlayGame(width, height, mines, input, output, bufferSize);

		output.SetCurrentFont(false, initialFontInfo);
		output.SetWindowBounds(true, intialWindowBounds);
		output.Write(L"\n");
		auto pos = output.GetCursorPosition();
		pos.X = 0;
		output.FillOutput(initialAttribute, (bufferSize.Height - pos.Y) * bufferSize.Width, pos);
		output.SetTextAttribute(initialAttribute);

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
			auto ev = input.ReadInput().AsKeyEvent();
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

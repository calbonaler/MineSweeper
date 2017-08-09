﻿#include <iostream>
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
		if (table(loc).flag)
			return true;
		if (!minesInitialized)
		{
			SetMines(loc);
			minesInitialized = true;
		}
		if (table(loc).open)
			return true;
		table(loc).open = true;
		if (table(loc).mine)
		{
			OpenAllMines();
			return false;
		}
		if (table(loc).mines > 0)
			return true;
		table.Around(loc, [this](const Location& loc) { OpenCell(loc); });
		return true;
	}
	bool OpenCellsWithCurrentMineIndicator(const Location& loc)
	{
		if (!table(loc).open)
			return true;
		size_t allArounds = 0;
		std::vector<Location> locs;
		table.Around(loc, [this, &allArounds, &locs](const Location& loc)
		{
			if (!table(loc).flag)
				locs.emplace_back(loc);
			allArounds++;
		});
		if (locs.size() != allArounds - table(loc).mines)
			return true;
		for (const auto& it : locs)
		{
			if (!OpenCell(it))
				return false;
		}
		return true;
	}
	void SwitchFlag(const Location& loc)
	{
		if (!table(loc).open)
			table(loc).flag = !table(loc).flag;
	}
	void SetCellPressedState(const Location& loc, bool pressed)
	{
		table.Around(loc, [this, pressed](const Location& loc)
		{
			if (!table(loc).flag)
				table(loc).pressed = pressed;
		});
	}
	bool IsValidLocation(const Location& loc) const { return table.IsValidLocation(loc); }
	bool HasCompleted() const
	{
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (!table(loc).mine && !table(loc).open)
				return false;
		}
		return true;
	}
	ptrdiff_t CountUnflagedMines() const
	{
		ptrdiff_t allMines = static_cast<ptrdiff_t>(mines);
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (table(loc).flag)
				--allMines;
		}
		return allMines;
	}

private:
	class Cell
	{
	public:
		Cell() : mines(0), open(false), mine(false), flag(false), pressed(false) { }
		uint8_t mines : 4;
		uint8_t open : 1;
		uint8_t mine : 1;
		uint8_t flag : 1;
		uint8_t pressed : 1;

		void Render(OutputConsole& output) const
		{
			if (!open)
			{
				if (!flag)
				{
					output.SetTextAttribute({ pressed ? ConsoleColor::Black : ConsoleColor::Gray, DefaultBackground });
					output.Write(L"■");
					output.SetTextAttribute({ DefaultForeground, DefaultBackground });
				}
				else
				{
					output.SetTextAttribute({ ConsoleColor::Red, DefaultBackground });
					output.Write(L"★");
					output.SetTextAttribute({ DefaultForeground, DefaultBackground });
				}
			}
			else if (mine)
				output.Write(L"●");
			else if (mines == 0)
				output.Write(L"  ");
			else
			{
				output.SetTextAttribute({ GetColor(mines), DefaultBackground });
				auto ch = static_cast<WCHAR>(L'０' + mines);
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
			if (without == loc || table(loc).mine)
				continue;
			table(loc).mine = true;
			i++;
		}
		for (Location loc; IsValidLocation(loc); loc = table.GetNextLocation(loc))
			table(loc).mines = CountMines(loc);
	}
	int CountMines(const Location& center) const
	{
		int mines = 0;
		table.Around(center, [this, &mines](const Location& moved) { if (table(moved).mine) mines++; });
		return mines;
	}
	void OpenAllMines()
	{
		for (Location loc; table.IsValidLocation(loc); loc = table.GetNextLocation(loc))
		{
			if (table(loc).mine)
				table(loc).open = true;
		}
	}
};

bool PlayGame(size_t width, size_t height, size_t mines, InputConsole& input, OutputConsole& output, ConsoleSize bufferSize)
{
	Game game(width, height, mines);
	std::optional<MouseButtonState> prevButtonState;
	std::optional<Location> pressedMarkPos;
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
			output.Write(L"残り地雷数: " + std::to_wstring(game.CountUnflagedMines()));
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
				if (pressedMarkPos)
				{
					game.SetCellPressedState(*pressedMarkPos, false);
					pressedMarkPos = std::nullopt;
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
				game.SwitchFlag(loc);
				res = true;
				renderRequested = true;
			}
			if (!prevButtonState->GetLeft() && !prevButtonState->GetRight() && ev->ButtonState.GetLeft() && !ev->ButtonState.GetRight())
				canOpenCell = true;
			if ((!prevButtonState->GetLeft() || !prevButtonState->GetRight()) && ev->ButtonState.GetLeft() && ev->ButtonState.GetRight())
			{
				pressedMarkPos = loc;
				game.SetCellPressedState(loc, true);
				renderRequested = true;
			}
			if (ev->Kind == MouseEventKind::Moved && pressedMarkPos && *pressedMarkPos != loc)
			{
				game.SetCellPressedState(*pressedMarkPos, false);
				pressedMarkPos = loc;
				game.SetCellPressedState(loc, true);
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

	while (true)
	{
		output.Write(L"幅: ");
		auto width = std::stoul(input.Read());
		output.Write(L"高さ: ");
		auto height = std::stoul(input.Read());
		unsigned long mines;
		while (true)
		{
			output.Write(L"地雷数: ");
			mines = std::stoul(input.Read());
			if (mines < width * height) break;
			output.SetTextAttribute({ ConsoleColor::Red, initialAttribute.Background });
			output.Write(L"地雷数は幅×高さよりも小さくなければなりません。\n");
			output.SetTextAttribute(initialAttribute);
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

		output.Write(L"もう一度挑戦する場合は [R] を、終了する場合は [Q] を押してください\n");
		while (true)
		{
			auto ev = input.ReadInput().AsKeyEvent();
			if (!ev) continue;
			if (ev->Char == L'R' || ev->Char == 'r') break;
			if (ev->Char == L'Q' || ev->Char == 'q') goto Exit;
		}
	}

Exit:;
}

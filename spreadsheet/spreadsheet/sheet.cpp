#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) <= pos.row) {
        sheet.resize(pos.row + 1);
    }
    if (static_cast<int>(sheet[pos.row].size()) <= pos.col) {
        sheet[pos.row].resize(pos.col + 1);
    }
    //ячейка может перезаписываться!!!
    if (!sheet[pos.row][pos.col]) {
        sheet[pos.row][pos.col] = std::make_unique<Cell>(*this, pos);
    }
    sheet[pos.row][pos.col]->Set(text);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) > pos.row && static_cast<int>(sheet[pos.row].size()) > pos.col && sheet[pos.row][pos.col]) {
        return sheet[pos.row][pos.col].get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) > pos.row && static_cast<int>(sheet[pos.row].size()) > pos.col && sheet[pos.row][pos.col]) {
        return sheet[pos.row][pos.col].get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) > pos.row && static_cast<int>(sheet[pos.row].size()) > pos.col && sheet[pos.row][pos.col]) {
        sheet[pos.row][pos.col] = nullptr;
    }
}

Size Sheet::GetPrintableSize() const {
    Size size{ 0,0 };
    if (sheet.size() == 0) {
        return size;
    }
    for (int row_index = 1; row_index <= static_cast<int>(sheet.size()); ++row_index) {
        for (int col_index = static_cast<int>(sheet[row_index - 1].size()); col_index > 0; --col_index) {
            if (sheet[row_index - 1][col_index - 1] && sheet[row_index - 1][col_index - 1]->GetText() != "") {
                if (col_index > size.cols) {
                    size.cols = col_index;
                }
                size.rows = row_index;
            }
        }
    }
    return size;
}

inline std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value);

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row_index = 0; row_index < size.rows; ++row_index) {
        bool IsFirst = true;
        for (int col_index = 0; col_index < size.cols; ++col_index) {
            if (!IsFirst) {
                output << '\t';
            }
            IsFirst = false;
            if (col_index < static_cast<int>(sheet[row_index].size()) && sheet[row_index][col_index] && !sheet[row_index][col_index]->GetText().empty()) {
                output << sheet[row_index][col_index]->GetValue();
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int row_index = 0; row_index < size.rows; ++row_index) {
        bool IsFirst = true;
        for (int col_index = 0; col_index < size.cols; ++col_index) {
            if (!IsFirst) {
                output << '\t';
            }
            IsFirst = false;
            if (col_index < static_cast<int>(sheet[row_index].size()) && sheet[row_index][col_index] && !sheet[row_index][col_index]->GetText().empty()) {
                output << sheet[row_index][col_index]->GetText();
            }
        }
        output << '\n';
    }
}

Cell* Sheet::GetPureCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) > pos.row && static_cast<int>(sheet[pos.row].size()) > pos.col && sheet[pos.row][pos.col]) {
        return sheet[pos.row][pos.col].get();
    }
    return nullptr;
}

const Cell* Sheet::GetPureCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
    if (static_cast<int>(sheet.size()) > pos.row && static_cast<int>(sheet[pos.row].size()) > pos.col && sheet[pos.row][pos.col]) {
        return sheet[pos.row][pos.col].get();
    }
    return nullptr;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
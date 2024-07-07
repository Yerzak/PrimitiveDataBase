#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"
#include <functional>
#include <unordered_set>

class Sheet;
class PositionHash {
public:
    size_t operator()(const Position p) const {
        return std::hash<std::string>()(p.ToString());
    }
};
class PositionPred {
public:
    bool operator()(const Position lhs, const Position rhs) const {
        return lhs == rhs;
    }
};
class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void Invalidate(std::unordered_set<Position, PositionHash, PositionPred>& set_of_cells);
    std::unordered_set<Position, PositionHash, PositionPred>& GetCellsWithReferences();
    void EraseFromParentSet(Position position);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> obj;
    // Добавьте поля и методы для связи с таблицей, проверки циклических 
    // зависимостей, графа зависимостей и т. д.
    Sheet& sheet_;
    std::unordered_set<Position, PositionHash, PositionPred> referenced_cells;
    std::unordered_set<Position, PositionHash, PositionPred> cells_with_references;
    Position pos_;
    bool IsCycled(std::unordered_set<Position, PositionHash, PositionPred>& container, Position needed_pos);
};

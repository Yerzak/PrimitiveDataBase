#include "cell.h"
#include "sheet.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <optional>

//---------------------------------------IMPL-CHILDREN------------------------
class Cell::Impl {
public:
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() = 0;
    virtual std::string GetText() = 0;
    virtual std::vector<Position> GetReferencedCells() = 0;
    virtual std::optional<FormulaInterface::Value> GetCache() = 0;
    virtual void MakeInvalid() = 0;
};
class Cell::EmptyImpl final : public Cell::Impl {
public:
    EmptyImpl() {
    }

    CellInterface::Value GetValue() override {
        return { "" };
    }
    std::string GetText() override {
        return "";
    }
    std::vector<Position> GetReferencedCells() override {
        return {};
    }
    std::optional<FormulaInterface::Value> GetCache() override {
        return std::nullopt;
    }
    void MakeInvalid() override {
    }
};
class Cell::TextImpl final : public Cell::Impl {
public:
    TextImpl(std::string text) {
        buf_text = text;
    }
    CellInterface::Value GetValue() override {
        if (buf_text[0] == '\'') {
            std::string temp_text(buf_text.begin() + 1, buf_text.end());
            return { temp_text };
        }
        return { buf_text };
    }
    std::string GetText() override {
        return buf_text;
    }
    std::vector<Position> GetReferencedCells() override {
        return {};
    }
    std::optional<FormulaInterface::Value> GetCache() override {
        return std::nullopt;
    }
    void MakeInvalid() override {
    }
private:
    std::string buf_text;
};
class Cell::FormulaImpl final : public Cell::Impl {
public:
    FormulaImpl(std::string text, const SheetInterface& sheet)
        : sheet_(sheet) {
        text.erase(text.begin());
        formula = ParseFormula(text);
    }
    CellInterface::Value GetValue() override {
        if (!cache.has_value()) {
            cache = formula->Evaluate(sheet_);
        }
        if (std::holds_alternative<double>(cache.value())) {
            return { std::get<double>(cache.value()) };
        }
        return { std::get<FormulaError>(cache.value()) };
    }
    std::string GetText() override {
        return std::string('=' + formula->GetExpression());
    }
    std::vector<Position> GetReferencedCells() {
        return formula->GetReferencedCells();
    }
    std::optional<FormulaInterface::Value> GetCache() override {
        return cache;
    }
    void MakeInvalid() {
        cache = std::nullopt;
    }
private:
    std::unique_ptr<FormulaInterface> formula;
    const SheetInterface& sheet_;
    mutable std::optional<FormulaInterface::Value> cache;
};
//-----------------------------------------------------------------------------------

// Реализуйте следующие методы
Cell::Cell(Sheet& sheet, Position pos)
    : obj(std::make_unique<EmptyImpl>()), sheet_(sheet), pos_(pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("In empty c-tor Cell");
    }
}
Cell::~Cell() {}

void Cell::Set(std::string text) {//ячейка может перезаписываться
    //повторный вызов метода с теми же аргументами за О(1)
    if (text == GetText()) {
        return;
    }
    //у ячеек, от которых зависела текущая, из cells_with_references нужно удалить позицию текущей  
    if (!referenced_cells.empty()) {
        for (auto cur_pos : referenced_cells) {
            sheet_.GetPureCell(cur_pos)->EraseFromParentSet(pos_);
        }
    }
    //изменить obj
    if (text == "") {
        obj = std::make_unique<EmptyImpl>();
        referenced_cells.clear();
    }
    else if (text[0] == '=' && text.size() > 1) {
        std::unique_ptr<Impl> temp_obj = std::make_unique<FormulaImpl>(text, sheet_);
        // вычислить новый referenced_cells и проверить его на цикличность
        std::unordered_set<Position, PositionHash, PositionPred> temp_cells;
        for (auto elem : temp_obj->GetReferencedCells()) {
            if(elem.IsValid()) {
                temp_cells.insert(elem);
            }
        }
        if (IsCycled(temp_cells, pos_) || std::string('=' + pos_.ToString()) == text) {
            throw CircularDependencyException("Cell-c-tor");
            return;
        }
        //если не цикличен, перезаписать имеющийся referenced_cells на новый
        obj = std::move(temp_obj);
        referenced_cells.clear();
        referenced_cells = std::move(temp_cells);
        //у ячеек, зависящих от текущей, нужно инвалидировать кэш
        Invalidate(cells_with_references);
        //теперь нужно позицию текущей ячейки внести в поле cells_with_references ячеек,
        //от которых теперь зависит текущая
        if (!referenced_cells.empty()) {
            for (auto cur_pos : referenced_cells) {
                auto cur_pointer = sheet_.GetPureCell(cur_pos);
                if (!cur_pointer) {
                    //здесь пытаемся нулевому указатиелю задать пустую строку
                    //возможно, нужно сначала создать пустую ячейку
                    sheet_.SetCell(cur_pos, "");
                }
                sheet_.GetPureCell(cur_pos)->cells_with_references.insert(pos_);
            }
        }
    }
    else {
        obj = std::make_unique<TextImpl>(text);
        referenced_cells.clear();
    }
    //поля sheet_, cells_with_references и pos_ не меняются
}

void Cell::Clear() {
    obj = std::make_unique<EmptyImpl>();
}

CellInterface::Value Cell::GetValue() const {
    if (obj->GetCache().has_value()) {
        auto result = obj->GetCache().value();
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        return std::get<FormulaError>(result);
    }
    return obj->GetValue();
}
std::string Cell::GetText() const {
    return obj->GetText();
}
void Cell::Invalidate(std::unordered_set<Position, PositionHash, PositionPred>& set_of_cells) {
    obj->MakeInvalid();
    for (auto ref : cells_with_references) {
        auto cell_ptr = sheet_.GetPureCell(ref);
        if (cell_ptr->GetCellsWithReferences().empty()) {
            continue;
        }
        else {
            sheet_.GetPureCell(ref)->Invalidate({ cell_ptr->GetCellsWithReferences() });
        }
    }
}
std::vector<Position> Cell::GetReferencedCells() const {
    if(referenced_cells.empty()) {
        return {};
    }
    std::vector<Position> result;
    for (auto cur_pos : referenced_cells) {
        result.push_back(cur_pos);
    }
    auto iter = std::unique(result.begin(), result.end());
    result.erase(iter, result.end());
    return result;
}
std::unordered_set<Position, PositionHash, PositionPred>& Cell::GetCellsWithReferences() {
    return cells_with_references;
}

void Cell::EraseFromParentSet(Position position) {
    cells_with_references.erase(position);
}
bool Cell::IsCycled(std::unordered_set<Position, PositionHash, PositionPred>& container, Position needed_pos) {
    if(container.count(needed_pos)) {
        return true;
    }
    for (auto& ref : container) {
        if(!sheet_.GetPureCell(ref)) {
            continue;
        }
        auto ref_cells_vec = sheet_.GetPureCell(ref)->GetReferencedCells();
        std::unordered_set<Position, PositionHash, PositionPred> ref_cells_set;
        for(auto ref_cell : ref_cells_vec) {
            ref_cells_set.insert(ref_cell);
        }
        if (ref_cells_set.count(needed_pos)) {
            return true;
        }
        else if (ref_cells_set.empty()) {
            continue;
        }
        else {
            return IsCycled(ref_cells_set, needed_pos);
        }
    }
    return false;
}



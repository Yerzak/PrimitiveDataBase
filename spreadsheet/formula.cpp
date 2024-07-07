#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(ParseFormulaAST(std::move(expression))) {
        } catch (...) {
            throw FormulaException("Formula Exception");
        }
    Value Evaluate(const SheetInterface& sheet) const override {
        auto SheetFunc = std::function([&](const Position cur_pos) {
            if (!cur_pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }
            auto cell_ptr = sheet.GetCell(cur_pos);
            if (!cell_ptr) {
                return 0.;
            }
            auto cell_value = cell_ptr->GetValue();
            if (std::holds_alternative<double>(cell_value)) {
                return std::get<double>(cell_value);
            }
            if (std::holds_alternative<std::string>(cell_value)) {
                auto text_res = std::get<std::string>(cell_value);
                double number_res = 0;
                if (!text_res.empty()) {
                    std::istringstream input(text_res);
                    if (!(input >> number_res) || !input.eof()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                }               
                return number_res;
            }
            throw FormulaError(std::get<FormulaError>(cell_value));
            });


        try {
            return ast_.Execute(SheetFunc).value();
        } catch (FormulaError& error) {
            return error;
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const {
        std::set<Position> temp;
        for (Position cur_pos : ast_.GetCells()) {
            if (cur_pos.IsValid()) {
                temp.insert(cur_pos);
            }
        }
        return { temp.begin(), temp.end() };
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

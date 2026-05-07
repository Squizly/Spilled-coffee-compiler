#pragma once

#include "parser.h"

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

struct RuntimeValue {
    ParserValueType type;
    int intValue;
    double floatValue;

    RuntimeValue();

    static RuntimeValue fromInt(int value);
    static RuntimeValue fromFloat(double value);

    int asInt() const;
    double asFloat() const;
    std::string toString() const;
};

class InterpreterError : public std::runtime_error {
public:
    explicit InterpreterError(const std::string& message);
};

class Interpreter {
public:
    Interpreter(const std::vector<RpnElement>& rpn,
                const std::vector<VariableInfo>& variables,
                const std::vector<ConstantInfo>& constants);
    explicit Interpreter(const Parser& parser);

    void run();
    void run(std::istream& input, std::ostream& output);

    const RuntimeValue& getValue(int variableIndex) const;
    const RuntimeValue& getValue(const std::string& variableName) const;
    const RuntimeValue& getArrayValue(int variableIndex, int elementIndex) const;

    std::string dumpVariables() const;

private:
    enum class StackItemKind {
        Value,
        Address,
        Label
    };

    struct StackItem {
        StackItemKind kind;
        RuntimeValue value;
        int variableIndex;
        int elementIndex;
        int label;
    };

    struct RuntimeVariable {
        VariableInfo info;
        std::vector<RuntimeValue> cells;
    };

    std::vector<RpnElement> rpnString;
    std::vector<VariableInfo> variableTable;
    std::vector<ConstantInfo> constantTable;
    std::vector<RuntimeVariable> memory;
    std::vector<StackItem> stack;

    void initializeMemory();
    void executeOperation(int operationCode, int& instructionPointer,
                          std::istream& input, std::ostream& output);

    StackItem makeValueItem(const RuntimeValue& value) const;
    StackItem makeAddressItem(int variableIndex, int elementIndex) const;
    StackItem makeLabelItem(int label) const;

    StackItem popItem();
    RuntimeValue popValue();
    StackItem popAddress();
    int popLabel();

    RuntimeValue resolveValue(const StackItem& item) const;
    RuntimeValue readCell(int variableIndex, int elementIndex) const;
    void writeCell(int variableIndex, int elementIndex, const RuntimeValue& value);

    RuntimeValue makeDefaultValue(ParserValueType type) const;
    RuntimeValue constantToValue(const ConstantInfo& constant) const;
    RuntimeValue convertToType(const RuntimeValue& value, ParserValueType targetType) const;

    RuntimeValue applyBinaryArithmetic(int operationCode,
                                       const RuntimeValue& left,
                                       const RuntimeValue& right) const;
    RuntimeValue applyComparison(int operationCode,
                                 const RuntimeValue& left,
                                 const RuntimeValue& right) const;
    RuntimeValue applyUnaryMinus(const RuntimeValue& value) const;
    RuntimeValue applyFunction(int operationCode, const RuntimeValue& value) const;

    bool isFalse(const RuntimeValue& value) const;
    int checkedVariableIndex(int variableIndex) const;
    int checkedElementIndex(int variableIndex, int elementIndex) const;
    int findVariable(const std::string& variableName) const;

    void runtimeError(const std::string& message) const;
};

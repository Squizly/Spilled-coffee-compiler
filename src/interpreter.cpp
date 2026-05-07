#include "../include/interpreter.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

RuntimeValue::RuntimeValue()
    : type(ParserValueType::Int), intValue(0), floatValue(0.0) {
}

RuntimeValue RuntimeValue::fromInt(int value) {
    RuntimeValue result;
    result.type = ParserValueType::Int;
    result.intValue = value;
    result.floatValue = static_cast<double>(value);
    return result;
}

RuntimeValue RuntimeValue::fromFloat(double value) {
    RuntimeValue result;
    result.type = ParserValueType::Float;
    result.intValue = static_cast<int>(value);
    result.floatValue = value;
    return result;
}

int RuntimeValue::asInt() const {
    if (type == ParserValueType::Int) {
        return intValue;
    }
    return static_cast<int>(floatValue);
}

double RuntimeValue::asFloat() const {
    if (type == ParserValueType::Int) {
        return static_cast<double>(intValue);
    }
    return floatValue;
}

std::string RuntimeValue::toString() const {
    std::ostringstream out;

    if (type == ParserValueType::Int) {
        out << intValue;
    } else {
        out << std::setprecision(10) << floatValue;
    }

    return out.str();
}

InterpreterError::InterpreterError(const std::string& message)
    : std::runtime_error("Interpreter error: " + message) {
}

Interpreter::Interpreter(const std::vector<RpnElement>& rpn,
                         const std::vector<VariableInfo>& variables,
                         const std::vector<ConstantInfo>& constants)
    : rpnString(rpn),
      variableTable(variables),
      constantTable(constants) {
    initializeMemory();
}

Interpreter::Interpreter(const Parser& parser)
    : rpnString(parser.getRpn()),
      variableTable(parser.getVariables()),
      constantTable(parser.getConstants()) {
    initializeMemory();
}

void Interpreter::run() {
    run(std::cin, std::cout);
}

void Interpreter::run(std::istream& input, std::ostream& output) {
    initializeMemory();
    stack.clear();

    int instructionPointer = 0;
    while (instructionPointer < static_cast<int>(rpnString.size())) {
        const RpnElement& element = rpnString[instructionPointer];
        ++instructionPointer;

        switch (element.type) {
            case ElementType::Address: {
                int variableIndex = checkedVariableIndex(element.value);
                int elementIndex =
                    (variableTable[variableIndex].form == VariableForm::Array) ? -1 : 0;
                stack.push_back(makeAddressItem(variableIndex, elementIndex));
                break;
            }
            case ElementType::Constant:
                if (element.value < 0 ||
                    element.value >= static_cast<int>(constantTable.size())) {
                    runtimeError("constant index is out of range");
                }
                stack.push_back(makeValueItem(constantToValue(constantTable[element.value])));
                break;
            case ElementType::Label:
                if (element.value < 0 ||
                    element.value > static_cast<int>(rpnString.size())) {
                    runtimeError("jump label is not resolved");
                }
                stack.push_back(makeLabelItem(element.value));
                break;
            case ElementType::Operation:
                executeOperation(element.value, instructionPointer, input, output);
                break;
        }
    }
}

const RuntimeValue& Interpreter::getValue(int variableIndex) const {
    int checkedIndex = checkedVariableIndex(variableIndex);
    if (variableTable[checkedIndex].form != VariableForm::Scalar) {
        runtimeError("variable is an array, not a scalar");
    }
    return memory[checkedIndex].cells[0];
}

const RuntimeValue& Interpreter::getValue(const std::string& variableName) const {
    int variableIndex = findVariable(variableName);
    if (variableIndex < 0) {
        runtimeError("unknown variable '" + variableName + "'");
    }
    return getValue(variableIndex);
}

const RuntimeValue& Interpreter::getArrayValue(int variableIndex, int elementIndex) const {
    int checkedIndex = checkedVariableIndex(variableIndex);
    if (variableTable[checkedIndex].form != VariableForm::Array) {
        runtimeError("variable is a scalar, not an array");
    }
    int checkedElement = checkedElementIndex(checkedIndex, elementIndex);
    return memory[checkedIndex].cells[checkedElement];
}

std::string Interpreter::dumpVariables() const {
    std::ostringstream out;

    for (std::size_t i = 0; i < memory.size(); ++i) {
        if (i != 0) {
            out << '\n';
        }

        out << memory[i].info.name << " = ";
        if (memory[i].info.form == VariableForm::Scalar) {
            out << memory[i].cells[0].toString();
        } else {
            out << '[';
            for (std::size_t j = 0; j < memory[i].cells.size(); ++j) {
                if (j != 0) {
                    out << ", ";
                }
                out << memory[i].cells[j].toString();
            }
            out << ']';
        }
    }

    return out.str();
}

void Interpreter::initializeMemory() {
    memory.clear();
    memory.reserve(variableTable.size());

    for (const VariableInfo& variable : variableTable) {
        RuntimeVariable runtimeVariable;
        runtimeVariable.info = variable;

        int cellCount = (variable.form == VariableForm::Array) ? variable.size : 1;
        if (cellCount <= 0) {
            runtimeError("variable '" + variable.name + "' has invalid size");
        }

        runtimeVariable.cells.assign(static_cast<std::size_t>(cellCount),
                                     makeDefaultValue(variable.type));
        memory.push_back(runtimeVariable);
    }
}

void Interpreter::executeOperation(int operationCode,
                                   int& instructionPointer,
                                   std::istream& input,
                                   std::ostream& output) {
    switch (operationCode) {
        case RPN_OP_ADD:
        case RPN_OP_SUB:
        case RPN_OP_MUL:
        case RPN_OP_DIV: {
            RuntimeValue right = popValue();
            RuntimeValue left = popValue();
            stack.push_back(makeValueItem(applyBinaryArithmetic(operationCode, left, right)));
            break;
        }
        case RPN_OP_EQ:
        case RPN_OP_NE:
        case RPN_OP_LT:
        case RPN_OP_GT:
        case RPN_OP_LE:
        case RPN_OP_GE: {
            RuntimeValue right = popValue();
            RuntimeValue left = popValue();
            stack.push_back(makeValueItem(applyComparison(operationCode, left, right)));
            break;
        }
        case RPN_OP_ASSIGN: {
            RuntimeValue value = popValue();
            StackItem address = popAddress();
            writeCell(address.variableIndex, address.elementIndex, value);
            break;
        }
        case RPN_OP_UNARY_MINUS:
            stack.push_back(makeValueItem(applyUnaryMinus(popValue())));
            break;
        case RPN_OP_INDEX: {
            RuntimeValue indexValue = popValue();
            if (indexValue.type != ParserValueType::Int) {
                runtimeError("array index must be integer");
            }

            StackItem baseAddress = popAddress();
            int variableIndex = checkedVariableIndex(baseAddress.variableIndex);
            if (variableTable[variableIndex].form != VariableForm::Array ||
                baseAddress.elementIndex != -1) {
                runtimeError("indexing requires an array address");
            }

            int elementIndex = checkedElementIndex(variableIndex, indexValue.intValue);
            stack.push_back(makeAddressItem(variableIndex, elementIndex));
            break;
        }
        case RPN_OP_READ: {
            StackItem address = popAddress();
            int variableIndex = checkedVariableIndex(address.variableIndex);
            checkedElementIndex(variableIndex, address.elementIndex);

            RuntimeValue value;
            if (variableTable[variableIndex].type == ParserValueType::Int) {
                int inputValue = 0;
                if (!(input >> inputValue)) {
                    runtimeError("failed to read integer value");
                }
                value = RuntimeValue::fromInt(inputValue);
            } else {
                double inputValue = 0.0;
                if (!(input >> inputValue)) {
                    runtimeError("failed to read floating point value");
                }
                value = RuntimeValue::fromFloat(inputValue);
            }
            writeCell(address.variableIndex, address.elementIndex, value);
            break;
        }
        case RPN_OP_WRITE:
            output << popValue().toString() << '\n';
            break;
        case RPN_OP_SQRT:
        case RPN_OP_EXP:
        case RPN_OP_LOG:
            stack.push_back(makeValueItem(applyFunction(operationCode, popValue())));
            break;
        case RPN_OP_JUMP:
            instructionPointer = popLabel();
            break;
        case RPN_OP_JUMP_FALSE: {
            int label = popLabel();
            RuntimeValue condition = popValue();
            if (isFalse(condition)) {
                instructionPointer = label;
            }
            break;
        }
        default:
            runtimeError("unknown RPN operation");
            break;
    }
}

Interpreter::StackItem Interpreter::makeValueItem(const RuntimeValue& value) const {
    StackItem item;
    item.kind = StackItemKind::Value;
    item.value = value;
    item.variableIndex = -1;
    item.elementIndex = -1;
    item.label = -1;
    return item;
}

Interpreter::StackItem Interpreter::makeAddressItem(int variableIndex, int elementIndex) const {
    StackItem item;
    item.kind = StackItemKind::Address;
    item.value = RuntimeValue();
    item.variableIndex = variableIndex;
    item.elementIndex = elementIndex;
    item.label = -1;
    return item;
}

Interpreter::StackItem Interpreter::makeLabelItem(int label) const {
    StackItem item;
    item.kind = StackItemKind::Label;
    item.value = RuntimeValue();
    item.variableIndex = -1;
    item.elementIndex = -1;
    item.label = label;
    return item;
}

Interpreter::StackItem Interpreter::popItem() {
    if (stack.empty()) {
        runtimeError("runtime stack is empty");
    }

    StackItem item = stack.back();
    stack.pop_back();
    return item;
}

RuntimeValue Interpreter::popValue() {
    return resolveValue(popItem());
}

Interpreter::StackItem Interpreter::popAddress() {
    StackItem item = popItem();
    if (item.kind != StackItemKind::Address) {
        runtimeError("address operand expected");
    }

    checkedVariableIndex(item.variableIndex);
    return item;
}

int Interpreter::popLabel() {
    StackItem item = popItem();
    if (item.kind != StackItemKind::Label) {
        runtimeError("label operand expected");
    }
    if (item.label < 0 || item.label > static_cast<int>(rpnString.size())) {
        runtimeError("jump target is out of range");
    }
    return item.label;
}

RuntimeValue Interpreter::resolveValue(const StackItem& item) const {
    switch (item.kind) {
        case StackItemKind::Value:
            return item.value;
        case StackItemKind::Address:
            return readCell(item.variableIndex, item.elementIndex);
        case StackItemKind::Label:
            runtimeError("label cannot be used as a value");
            break;
    }

    return RuntimeValue();
}

RuntimeValue Interpreter::readCell(int variableIndex, int elementIndex) const {
    int checkedIndex = checkedVariableIndex(variableIndex);
    int checkedElement = checkedElementIndex(checkedIndex, elementIndex);
    return memory[checkedIndex].cells[checkedElement];
}

void Interpreter::writeCell(int variableIndex, int elementIndex, const RuntimeValue& value) {
    int checkedIndex = checkedVariableIndex(variableIndex);
    int checkedElement = checkedElementIndex(checkedIndex, elementIndex);
    memory[checkedIndex].cells[checkedElement] =
        convertToType(value, variableTable[checkedIndex].type);
}

RuntimeValue Interpreter::makeDefaultValue(ParserValueType type) const {
    if (type == ParserValueType::Float) {
        return RuntimeValue::fromFloat(0.0);
    }
    return RuntimeValue::fromInt(0);
}

RuntimeValue Interpreter::constantToValue(const ConstantInfo& constant) const {
    if (constant.type == ParserValueType::Float) {
        char* endPointer = nullptr;
        double value = std::strtod(constant.value.c_str(), &endPointer);
        if (endPointer == constant.value.c_str() || *endPointer != '\0') {
            runtimeError("invalid floating point constant '" + constant.value + "'");
        }
        return RuntimeValue::fromFloat(value);
    }

    char* endPointer = nullptr;
    long value = std::strtol(constant.value.c_str(), &endPointer, 10);
    if (endPointer == constant.value.c_str() || *endPointer != '\0') {
        runtimeError("invalid integer constant '" + constant.value + "'");
    }
    if (value < std::numeric_limits<int>::min() ||
        value > std::numeric_limits<int>::max()) {
        runtimeError("integer constant is out of range");
    }
    return RuntimeValue::fromInt(static_cast<int>(value));
}

RuntimeValue Interpreter::convertToType(const RuntimeValue& value,
                                        ParserValueType targetType) const {
    if (targetType == ParserValueType::Float) {
        return RuntimeValue::fromFloat(value.asFloat());
    }
    return RuntimeValue::fromInt(value.asInt());
}

RuntimeValue Interpreter::applyBinaryArithmetic(int operationCode,
                                                const RuntimeValue& left,
                                                const RuntimeValue& right) const {
    bool floatResult =
        left.type == ParserValueType::Float || right.type == ParserValueType::Float;

    if (!floatResult) {
        int a = left.intValue;
        int b = right.intValue;

        switch (operationCode) {
            case RPN_OP_ADD:
                return RuntimeValue::fromInt(a + b);
            case RPN_OP_SUB:
                return RuntimeValue::fromInt(a - b);
            case RPN_OP_MUL:
                return RuntimeValue::fromInt(a * b);
            case RPN_OP_DIV:
                if (b == 0) {
                    runtimeError("division by zero");
                }
                return RuntimeValue::fromInt(a / b);
        }
    }

    double a = left.asFloat();
    double b = right.asFloat();
    double result = 0.0;

    switch (operationCode) {
        case RPN_OP_ADD:
            result = a + b;
            break;
        case RPN_OP_SUB:
            result = a - b;
            break;
        case RPN_OP_MUL:
            result = a * b;
            break;
        case RPN_OP_DIV:
            if (b == 0.0) {
                runtimeError("division by zero");
            }
            result = a / b;
            break;
        default:
            runtimeError("invalid arithmetic operation");
            break;
    }

    if (!std::isfinite(result)) {
        runtimeError("arithmetic overflow");
    }
    return RuntimeValue::fromFloat(result);
}

RuntimeValue Interpreter::applyComparison(int operationCode,
                                          const RuntimeValue& left,
                                          const RuntimeValue& right) const {
    bool result = false;

    if (left.type == ParserValueType::Float || right.type == ParserValueType::Float) {
        double a = left.asFloat();
        double b = right.asFloat();

        switch (operationCode) {
            case RPN_OP_EQ: result = (a == b); break;
            case RPN_OP_NE: result = (a != b); break;
            case RPN_OP_LT: result = (a < b); break;
            case RPN_OP_GT: result = (a > b); break;
            case RPN_OP_LE: result = (a <= b); break;
            case RPN_OP_GE: result = (a >= b); break;
            default: runtimeError("invalid comparison operation"); break;
        }
    } else {
        int a = left.intValue;
        int b = right.intValue;

        switch (operationCode) {
            case RPN_OP_EQ: result = (a == b); break;
            case RPN_OP_NE: result = (a != b); break;
            case RPN_OP_LT: result = (a < b); break;
            case RPN_OP_GT: result = (a > b); break;
            case RPN_OP_LE: result = (a <= b); break;
            case RPN_OP_GE: result = (a >= b); break;
            default: runtimeError("invalid comparison operation"); break;
        }
    }

    return RuntimeValue::fromInt(result ? 1 : 0);
}

RuntimeValue Interpreter::applyUnaryMinus(const RuntimeValue& value) const {
    if (value.type == ParserValueType::Float) {
        return RuntimeValue::fromFloat(-value.floatValue);
    }
    return RuntimeValue::fromInt(-value.intValue);
}

RuntimeValue Interpreter::applyFunction(int operationCode, const RuntimeValue& value) const {
    double argument = value.asFloat();
    double result = 0.0;

    switch (operationCode) {
        case RPN_OP_SQRT:
            if (argument < 0.0) {
                runtimeError("sqrt argument must be non-negative");
            }
            result = std::sqrt(argument);
            break;
        case RPN_OP_EXP:
            result = std::exp(argument);
            if (!std::isfinite(result)) {
                runtimeError("exp overflow");
            }
            break;
        case RPN_OP_LOG:
            if (argument <= 0.0) {
                runtimeError("log argument must be positive");
            }
            result = std::log(argument);
            break;
        default:
            runtimeError("invalid standard function");
            break;
    }

    return RuntimeValue::fromFloat(result);
}

bool Interpreter::isFalse(const RuntimeValue& value) const {
    if (value.type == ParserValueType::Float) {
        return value.floatValue == 0.0;
    }
    return value.intValue == 0;
}

int Interpreter::checkedVariableIndex(int variableIndex) const {
    if (variableIndex < 0 || variableIndex >= static_cast<int>(variableTable.size())) {
        runtimeError("variable index is out of range");
    }
    return variableIndex;
}

int Interpreter::checkedElementIndex(int variableIndex, int elementIndex) const {
    checkedVariableIndex(variableIndex);

    if (elementIndex < 0) {
        runtimeError("array element address expected");
    }

    if (elementIndex >= static_cast<int>(memory[variableIndex].cells.size())) {
        runtimeError("array index is out of bounds");
    }

    return elementIndex;
}

int Interpreter::findVariable(const std::string& variableName) const {
    for (std::size_t i = 0; i < variableTable.size(); ++i) {
        if (variableTable[i].name == variableName) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void Interpreter::runtimeError(const std::string& message) const {
    throw InterpreterError(message);
}

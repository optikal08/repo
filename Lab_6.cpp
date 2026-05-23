#include <iostream>
#include <string>
#include <cmath>
#include <cassert>

using namespace std;
 
struct Transformer;
struct Number;
struct BinaryOperation;
struct FunctionCall;
struct Variable;

struct Expression {
    virtual ~Expression() { } //виртуальный деструктор
    virtual double evaluate() const = 0; //абстрактный метод «вычислить»
    virtual Expression *transform(Transformer *tr) const = 0;
};

struct Transformer //pattern Visitor
{
    virtual ~Transformer() { }
    virtual Expression *transformNumber(Number const *) = 0;
    virtual Expression *transformBinaryOperation(BinaryOperation const *) = 0;
    virtual Expression *transformFunctionCall(FunctionCall const *) = 0;
    virtual Expression *transformVariable(Variable const *) = 0;
};

struct Number : Expression // структура «Число»
{
    Number(double value); //конструктор
    double value() const; // метод чтения значения числа
    double evaluate() const; // реализация виртуального метода «вычислить»
    Expression *transform(Transformer *tr) const;
private:
    double value_; // само вещественное число
};

struct BinaryOperation : Expression // «Бинарная операция»
{
    enum { // перечислим константы, которыми зашифруем символы операций
        PLUS = '+',
        MINUS = '-',
        DIV = '/',
        MUL = '*'
    };
    // в конструкторе надо указать 2 операнда – левый и правый, а также сам символ операции
    BinaryOperation(Expression const *left, int op, Expression const *right);
    ~BinaryOperation(); //в деструкторе освободим занятую память
    double evaluate() const; // реализация виртуального метода «вычислить»
    Expression *transform(Transformer *tr) const;
    Expression const *left() const; // чтение левого операнда
    Expression const *right() const; // чтение правого операнда
    int operation() const; // чтение символа операции
private:
    Expression const *left_; // указатель на левый операнд
    Expression const *right_; // указатель на правый операнд
    int op_; // символ операции
};

struct FunctionCall : Expression // структура «Вызов функции»
{
    // в конструкторе надо учесть имя функции и ее аргумент
    FunctionCall(std::string const &name, Expression const *arg);
    // разрешены только вызов sqrt и abs
    ~FunctionCall(); // освобождаем память в деструкторе
    double evaluate() const; // реализация виртуального метода «вычислить»
    Expression *transform(Transformer *tr) const;
    std::string const &name() const;
    Expression const *arg() const; // чтение аргумента функции
private:
    std::string const name_; // имя функции
    Expression const *arg_; // указатель на ее аргумент
};

struct Variable : Expression // структура «Переменная»
{
    Variable(std::string const name); // в конструкторе надо указать ее имя
    std::string const & name() const; // чтение имени переменной
    double evaluate() const; // реализация виртуального метода «вычислить»
    Expression *transform(Transformer *tr) const;
private:
    std::string const name_; // имя переменной
};

Number::Number(double value) : value_(value) {}
double Number::value() const { return value_; }
double Number::evaluate() const { return value_; }
Expression *Number::transform(Transformer *tr) const { return tr->transformNumber(this); }

BinaryOperation::BinaryOperation(Expression const *left, int op, Expression const *right)
    : left_(left), op_(op), right_(right) { assert(left_ && right_); }
BinaryOperation::~BinaryOperation() { delete left_; delete right_; }
double BinaryOperation::evaluate() const {
    double left = left_->evaluate(); // вычисляем левую часть
    double right = right_->evaluate(); // вычисляем правую часть
    switch (op_) // в зависимости от вида операции, складываем, вычитаем, умножаем или делим
    {
        case PLUS: return left + right;
        case MINUS: return left - right;
        case DIV: return left / right;
        case MUL: return left * right;
    }
    return 0.0;
}
Expression *BinaryOperation::transform(Transformer *tr) const { return tr->transformBinaryOperation(this); }
Expression const *BinaryOperation::left() const { return left_; }
Expression const *BinaryOperation::right() const { return right_; }
int BinaryOperation::operation() const { return op_; }

FunctionCall::FunctionCall(std::string const &name, Expression const *arg)
    : name_(name), arg_(arg) { assert(arg_); assert(name_ == "sqrt" || name_ == "abs"); }
FunctionCall::~FunctionCall() { delete arg_; }
double FunctionCall::evaluate() const {
    if (name_ == "sqrt") return sqrt(arg_->evaluate()); // либо вычисляем корень квадратный
    else return fabs(arg_->evaluate()); // либо модуль – остальные функции запрещены
}
Expression *FunctionCall::transform(Transformer *tr) const { return tr->transformFunctionCall(this); }
std::string const &FunctionCall::name() const { return name_; }
Expression const *FunctionCall::arg() const { return arg_; }

Variable::Variable(std::string const name) : name_(name) {}
std::string const &Variable::name() const { return name_; }
double Variable::evaluate() const { return 0.0; }
Expression *Variable::transform(Transformer *tr) const { return tr->transformVariable(this); }

//  Задание 1 
struct CopySyntaxTree : Transformer
{
    Expression *transformNumber(Number const *number) override
    {
        return new Number(number->value());
    }
    Expression *transformBinaryOperation(BinaryOperation const *binop) override
    {
        Expression *newLeft = binop->left()->transform(this);
        Expression *newRight = binop->right()->transform(this);
        return new BinaryOperation(newLeft, binop->operation(), newRight);
    }
    Expression *transformFunctionCall(FunctionCall const *fcall) override
    {
        Expression *newArg = fcall->arg()->transform(this);
        return new FunctionCall(fcall->name(), newArg);
    }
    Expression *transformVariable(Variable const *var) override
    {
        return new Variable(var->name());
    }
};

//  Задание 2 
struct FoldConstants : Transformer
{
    Expression *transformNumber(Number const *number) override
    {
        return new Number(number->value());
    }
    Expression *transformBinaryOperation(BinaryOperation const *binop) override
    {
        Expression *newLeft = binop->left()->transform(this);
        Expression *newRight = binop->right()->transform(this);
        Number *leftNum = dynamic_cast<Number*>(newLeft);
        Number *rightNum = dynamic_cast<Number*>(newRight);
        if (leftNum && rightNum) {
            double res;
            switch (binop->operation()) {
                case BinaryOperation::PLUS: res = leftNum->value() + rightNum->value(); break;
                case BinaryOperation::MINUS: res = leftNum->value() - rightNum->value(); break;
                case BinaryOperation::MUL: res = leftNum->value() * rightNum->value(); break;
                case BinaryOperation::DIV: res = leftNum->value() / rightNum->value(); break;
            }
            delete newLeft;
            delete newRight;
            return new Number(res);
        }
        return new BinaryOperation(newLeft, binop->operation(), newRight);
    }
    Expression *transformFunctionCall(FunctionCall const *fcall) override
    {
        Expression *newArg = fcall->arg()->transform(this);
        Number *argNum = dynamic_cast<Number*>(newArg);
        if (argNum) {
            double val = argNum->value();
            double res;
            if (fcall->name() == "sqrt") res = sqrt(val);
            else res = fabs(val);
            delete newArg;
            return new Number(res);
        }
        return new FunctionCall(fcall->name(), newArg);
    }
    Expression *transformVariable(Variable const *var) override
    {
        return new Variable(var->name());
    }
};

int main()
{
    Number* n32 = new Number(32.0);
    Number* n16 = new Number(16.0);
    BinaryOperation* minus = new BinaryOperation(n32, BinaryOperation::MINUS, n16);
    FunctionCall* callSqrt = new FunctionCall("sqrt", minus);
    Variable* var = new Variable("var");
    BinaryOperation* mult = new BinaryOperation(var, BinaryOperation::MUL, callSqrt);
    FunctionCall* callAbs = new FunctionCall("abs", mult);

    // Задание 1
    CopySyntaxTree cst;
    Expression* copy = callAbs->transform(&cst);
    cout << "Original: " << callAbs->evaluate() << endl;
    cout << "Copy: " << copy->evaluate() << endl;

    // Задание 2
    Number* n32b = new Number(32.0);
    Number* n16b = new Number(16.0);
    BinaryOperation* minus2 = new BinaryOperation(n32b, BinaryOperation::MINUS, n16b);
    FunctionCall* sqrt2 = new FunctionCall("sqrt", minus2);
    Variable* var2 = new Variable("var");
    BinaryOperation* mult2 = new BinaryOperation(var2, BinaryOperation::MUL, sqrt2);
    FunctionCall* abs2 = new FunctionCall("abs", mult2);

    FoldConstants fc;
    Expression* folded = abs2->transform(&fc);
    cout << "Folded evaluate: " << folded->evaluate() << endl;

    delete callAbs;
    delete copy;
    delete abs2;
    delete folded;

    return 0;
}
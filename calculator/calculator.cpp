#include <iostream>
#include <vector>
#include <string>
#include <cctype>
#include <stdexcept>
#include <map>
#include <cmath>
#include <algorithm>

using namespace std;

enum class TokenType {
    NUMBER,        // 数字（整数/小数）
    ID,            // 标识符（变量/函数名）
    OPERATOR,      // 运算符（+、-、*、/、**等）
    LPAREN,        // 左括号 (
    RPAREN,        // 右括号 )
    ASSIGN,        // 赋值 =
    END            // 结束符
};

struct Token {
    TokenType type;
    string value;
    double num_value;  // 仅NUMBER类型有效

    Token(TokenType t, const string& v, double nv = 0.0)
    {
        type = t;
        value = v;
        num_value = nv;
    }
};

class Lexer {
private:
    string input;
    size_t pos;
    char current_char;

    void advance() {
        pos++;
        current_char = (pos < input.size()) ? input[pos] : '\0';
    }


    Token tokenize_number() {
        string num_str;
        // 整数部分
        while (isdigit(current_char)) {
            num_str += current_char;
            advance();
        }
        // 小数部分
        if (current_char == '.') {
            num_str += current_char;
            advance();
            if (!isdigit(current_char)) {
                throw runtime_error("无效数字：小数点后无数字");
            }
            while (isdigit(current_char)) {
                num_str += current_char;
                advance();
            }
        }
        double num = stod(num_str);
        return Token(TokenType::NUMBER, num_str, num);
    }

    // 识别标识符（变量/函数名）
    Token tokenize_id() {
        string id_str;
        while (isalnum(current_char) || current_char == '_') {
            id_str += current_char;
            advance();
        }
        return Token(TokenType::ID, id_str);
    }

    // 识别运算符（最长匹配原则）
    Token tokenize_operator() {
        string op_str;
        op_str += current_char;
        advance();

        switch (op_str[0]) {
        case '=':
            return Token(TokenType::ASSIGN, "=");
            break;
        case '/':
            break;
        case '%':
            break;
        case '*':
            if (current_char == '*') { op_str += current_char; advance(); }
            break;

        }
        return Token(TokenType::OPERATOR, op_str);
    }

public:
    Lexer(const string& expr)
    {
        input = expr;
        pos = 0;
        current_char = (pos < input.size()) ? input[pos] : '\0';
    }

    // 生成Token列表（词法分析入口）
    vector<Token> tokenize() {
        vector<Token> tokens;
        while (current_char != '\0') {
            // 跳过空白符
            if (isspace(current_char)) {
                advance();
                continue;
            }
            // 数字
            else if (isdigit(current_char)) {
                tokens.push_back(tokenize_number());
            }
            // 标识符
            else if (isalpha(current_char) || current_char == '_') {
                tokens.push_back(tokenize_id());
            }
            // 运算符
            else if (current_char == '+' || current_char == '-' ||
                current_char == '*' || current_char == '/' ||
                current_char == '%' || current_char == '=') {
                tokens.push_back(tokenize_operator());
            }
            // 括号
            else if (current_char == '(') {
                tokens.push_back(Token(TokenType::LPAREN, "("));
                advance();
            }
            else if (current_char == ')') {
                tokens.push_back(Token(TokenType::RPAREN, ")"));
                advance();
            }
            // 未知字符
            else {
                throw runtime_error("未知字符:" + string(1, current_char));
            }
        }
        tokens.push_back(Token(TokenType::END, ""));
        return tokens;
    }
};

class ExpressionParser {
private:
    vector<Token> tokens;
    size_t token_pos;
    map<string, double> variables;  // 变量表

    // 前瞻当前Token
    Token peek() const {
        return (token_pos < tokens.size()) ? tokens[token_pos] : Token(TokenType::END, "");
    }

    // 消费当前Token
    void advance() {
        if (token_pos < tokens.size())
            token_pos++;
    }

    // 匹配指定Token
    bool match(TokenType type, const string& value = "") {
        Token current = peek();
        if (current.type == type && (value.empty() || current.value == value)) {
            advance();
            return true;
        }
        return false;
    }
    double parse_power() {
        double val = parse_factor();
        while (match(TokenType::OPERATOR, "**")) {
            double power_val = parse_power();
            val = pow(val, power_val);
        }
        return val;
    }

    // 解析因子（最基础原子：数字/变量/括号/一元运算符）
    double parse_factor() {
        //判断一元运算符
        if (match(TokenType::OPERATOR, "+")) {
            return parse_factor();
        }
        else if (match(TokenType::OPERATOR, "-")) {
            return -parse_factor();
        }
        else if (match(TokenType::OPERATOR, "!")) {
            double val = parse_factor();
            return (val == 0.0) ? 1.0 : 0.0;
        }
        // 数字
        else if (match(TokenType::NUMBER)) {
            return tokens[token_pos - 1].num_value;
        }
        // 变量
        else if (match(TokenType::ID)) {
            string id = tokens[token_pos - 1].value;
            if (variables.find(id) != variables.end()) {
                return variables[id];
            }
            else {
                throw runtime_error("未定义变量：" + id);
            }
        }
        // 括号表达式
        else if (match(TokenType::LPAREN)) {
            double val = parse_expr();
            if (!match(TokenType::RPAREN)) {
                throw runtime_error("语法错误：缺少右括号 )");
            }
            return val;
        }

        throw runtime_error("语法错误：期望数字/变量/括号/一元运算符，实际是：" + peek().value);
    }

    double parse_term() {
        double val = parse_power();

        while (true) {
            Token current = peek();
            if (match(TokenType::OPERATOR, "*") || match(TokenType::OPERATOR, "/") || match(TokenType::OPERATOR, "%"))
            {
                string op = current.value;
                double power_val = parse_power();

                if (op == "*") val *= power_val;
                else if (op == "/") {
                    if (power_val == 0) throw runtime_error("运行错误：除数不能为0");
                    val /= power_val;
                }
                else if (op == "%") {
                    int temp = (int)val;
                    temp %= (int)power_val;
                    val = temp;
                }
            }
            else {
                break;
            }
        }
        return val;
    }

    // 解析表达式
    double parse_expr() {
        double val = parse_term();
        while (true) {
            Token current = peek();
            if (match(TokenType::OPERATOR, "+") || match(TokenType::OPERATOR, "-")) {
                string op = current.value;
                double term_val = parse_term();

                if (op == "+") val += term_val;
                else if (op == "-") val -= term_val;
            }
            else {
                break;
            }
        }
        return val;
    }
    bool parse_assignment() {
        size_t original_pos = token_pos;
        Token current = peek();

        if (current.type != TokenType::ID) return false;
        advance();

        if (match(TokenType::ASSIGN, "=")) {
            double val = parse_expr();
            variables[current.value] = val;
            cout << current.value << " = " << val << endl;
            return true;
        }
        else {
            token_pos = original_pos;
            return false;
        }
    }



public:
    ExpressionParser() {
        token_pos = 0;
        variables["pi"] = acos(-1.0);
        variables["e"] = exp(1.0);
    }

    // 计算表达式入口
    double calculate(const string& expr) {
        // 重置Token指针
        token_pos = 0;
        // 词法分析
        Lexer lexer(expr);
        tokens = lexer.tokenize();

        // 先解析赋值语句
        if (parse_assignment()) {
            return 0.0;
        }

        // 解析普通表达式
        double result = parse_expr();
        if (peek().type != TokenType::END) {
            throw runtime_error("语法错误：表达式末尾有多余字符：" + peek().value);
        }
        return result;
    }

    void print_variables() {
        cout << "已定义变量：" << endl;
        for (auto it = variables.begin(); it != variables.end(); ++it) {
            cout << "  " << it->first << " = " << it->second << endl;
        }
    }
};
void repl() {
    ExpressionParser calculator;
    cout << "==================== 表达式计算器 ====================" << endl;
    cout << "支持功能：" << endl;
    cout << "  1. 基础运算：+ - * / **（幂运算，右结合）" << endl;
    cout << "  2. 括号优先级：()" << endl;
    cout << "  3. 一元运算符：+（正）、-（负）、!（非，0→1，非0→0）" << endl;
    cout << "  4. 内置常量：pi（圆周率）、e（自然常数）" << endl;
    cout << "  5. 自定义变量：如 x=10、y=pi+2" << endl;
    cout << "  6. 查看变量：输入 vars" << endl;
    cout << "退出：输入 exit" << endl;
    cout << "======================================================" << endl;

    string input;
    while (true) {
        cout << "> ";
        getline(cin, input);

        // 去除所有空格
        auto new_end = remove_if(input.begin(), input.end(), [](unsigned char c) {
            return isspace(c);
            });
        input.erase(new_end, input.end());

        // 空输入跳过
        if (input.empty()) continue;

        // 退出
        if (input == "exit") {
            cout << "计算器已退出" << endl;
            break;
        }

        // 查看变量
        if (input == "vars") {
            calculator.print_variables();
            continue;
        }

        // 计算表达式
        try {
            double result = calculator.calculate(input);
            // 仅非赋值语句输出结果
            if (input.find('=') == string::npos) {
                cout << "结果：" << result << endl;
            }
        }
        catch (const exception& e) {
            cerr << "错误：" << e.what() << endl;
        }
    }
}

// 主函数
int main() {
    repl();
    return 0;
}
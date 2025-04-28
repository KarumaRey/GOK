from nodes import *

IDENT = "IDENT"
NUMBER = "NUMBER"
WHILE = "WHILE"
IF = "IF"
ELSE = "ELSE"
L_FIG = "L_FIG"
R_FIG = "R_FIG"
LEFT_PAREN = "LEFT_PAREN"
RIGHT_PAREN = "RIGHT_PAREN"
PLUS = "PLUS"
MINUS = "MINUS"
EQ = "EQ"
RETURN = "RETURN"
END = "END"

class Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.curToken = None

    def next_token(self):
        # Переходит к следующему токену в списке токенов.
        self.curToken = next(self.tokens)

    def ass(self, token):
        # Проверяет, что текущий токен имеет заданный тип (token).
        assert self.curToken.token_type == token

    def ass_next(self, token):
        # Проверяет, что текущий токен имеет заданный тип (token) и переходит к следующему токену.
        self.ass(token)
        self.next_token()

    def parse_function(self):
        # Разбирает функцию.
        self.next_token()
        self.ass(IDENT)
        self.next_token()
        self.ass_next(L_FIG)
        body = self.parse_block()
        self.ass_next(RETURN)
        return_st = self.parse_atom()
        self.ass_next(R_FIG)
        self.ass(END)
        return Function(body, return_st)

    def parse_atom(self):
        # Разбирает атомарное выражение.
        if self.curToken.token_type == NUMBER:
            return self.parse_number()
        elif self.curToken.token_type == IDENT:
            return self.parse_ident()
        elif self.curToken.token_type == LEFT_PAREN:
            return self.parse_par()
        else:
            return 0

    def parse_number(self):
        # Разбирает числовое значение.
        result = Number(self.curToken.value)
        self.next_token()
        return result

    def parse_ident(self):
        # Разбирает идентификатор переменной.
        result = Ident(self.curToken.value)
        self.next_token()
        return result

    def parse_par(self):
        # Разбирает выражение в скобках.
        self.ass_next(LEFT_PAREN)
        expr = self.parse_bin()
        self.ass_next(RIGHT_PAREN)
        return expr

    def parse_bin(self):
        # Разбирает бинарное выражение.
        left = self.parse_atom()
        return self.parse_bin_r(left)

    def parse_bin_r(self, left):
        # Рекурсивно разбирает бинарное выражение с операторами + и -.
        if self.curToken.token_type != PLUS and self.curToken.token_type != MINUS:
            return left
        op = self.curToken.value
        self.next_token()
        right = self.parse_atom()
        if self.curToken.token_type == PLUS or self.curToken.token_type == MINUS:
            right = self.parse_bin_r(right)
        return BinOp(op, left, right)

    def parse_assign(self):
        # Разбирает операцию присваивания.
        var = self.parse_ident()
        self.ass_next(EQ)
        value = self.parse_bin()
        return Assign(var, value)

    def parse_if(self):
        # Разбирает условное выражение if-else.
        self.next_token()
        cond = self.parse_par()
        self.ass_next(L_FIG)
        then_block = self.parse_block()
        self.ass_next(R_FIG)
        if self.curToken.token_type == ELSE:
            self.next_token()
            self.ass_next(L_FIG)
            else_block = self.parse_block()
            self.ass_next(R_FIG)
            return IfExpr(cond, then_block, else_block)
        return IfExpr(cond, then_block, None, False)

    def parse_while(self):
        # Разбирает цикл while.
        self.next_token()
        cond = self.parse_par()
        self.ass_next(L_FIG)
        body = self.parse_block()
        self.ass_next(R_FIG)
        return WhileExpr(cond, body)

    def parse_block(self):
        # Разбирает блок выражений.
        exprs = []
        while True:
            if self.curToken.token_type == IF:
                exprs.append(self.parse_if())
            elif self.curToken.token_type == WHILE:
                exprs.append(self.parse_while())
            elif self.curToken.token_type == IDENT:
                exprs.append(self.parse_assign())
            else:
                break
        return Block(exprs)

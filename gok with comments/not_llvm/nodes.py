#определения классов и функций для создания и работы с IR
from context import *
from builder import *

# Создание экземпляров классов Context и Builder
context = Context()
builder = Builder(context)


class Assign:
    def __init__(self, variable, value):
        # Конструктор класса Assign. Принимает переменную (variable) и значение (value) для присваивания.
        self.variable = variable
        self.value = value

    def __str__(self):
        # Возвращает строковое представление присваивания.
        return str(self.variable) + " = " + str(self.value)

    def generate(self):
        # Генерирует промежуточное представление (IR) для присваивания.
        name = self.variable.name
        builder.context.names.add(name)
        value = self.value.generate()
        expr = IR.IR(IR.ASSIGN, name)
        if isinstance(value, BinOp):
            expr.add_argument(value.l)
            expr.add_argument(value.r)
            expr.add_bin_op(value.op)
        else:
            expr.add_argument(value)
        return expr


class Block:
    def __init__(self, exprs):
        # Конструктор класса Block. Принимает список выражений (exprs) в блоке.
        self.exprs = exprs

    def __str__(self):
        # Возвращает строковое представление блока и его выражений.
        result = "block(\n"
        for expr in self.exprs:
            result += str(expr) + "\n"
        return result + ")"

    def generate(self):
        # Генерирует промежуточное представление (IR) для блока.
        for expr in self.exprs:
            e = expr.generate()
            if e.type != IR.NULL:
                builder.add_expression(e)
        return IR.IR.create_empty_expr()


class BinOp:
    def __init__(self, op, l, r):
        # Конструктор класса BinOp. Принимает оператор (op), левый операнд (l) и правый операнд (r).
        self.op = op
        self.l = l
        self.r = r

    def __str__(self):
        # Возвращает строковое представление бинарной операции.
        return str(self.l) + " " + self.op + " " + str(self.r)

    def generate(self):
        # Генерирует промежуточное представление (IR) для бинарной операции.
        l = self.l.generate()
        r = self.r.generate()
        return BinOp(self.op, l, r)


class IfExpr:
    def __init__(self, cond, then_br, else_br, has_else=True):
        # Конструктор класса IfExpr. Принимает условие (cond), блок then-ветви (then_br),
        # блок else-ветви (else_br) и флаг наличия блока else (has_else).
        self.cond = cond
        self.then_br = then_br
        self.else_br = else_br
        self.has_else = has_else

    def __str__(self):
        # Возвращает строковое представление условного выражения.
        if self.has_else:
            return "if (" + str(self.cond) + ") {" + str(self.then_br) + "} else {" + str(self.else_br) + "}"
        else:
            return "if (" + str(self.cond) + ") {" + str(self.then_br) + "}"

    def generate(self):
        # Генерирует промежуточное представление (IR) для условного выражения.
        cond_block = builder.current_block()
        cond_expr = IR.IR(IR.CMP, " ")
        e = self.cond.generate()
        if isinstance(e, BinOp):
            cond_expr.add_argument(e.l)
            cond_expr.add_argument(e.r)
            cond_expr.add_bin_op(e.op)
        else:
            cond_expr.add_argument(e)
        builder.add_expression(cond_expr)
        builder.create_block()
        self.then_br.generate()
        after_block = builder.create_block()
        if self.has_else:
            builder.set_insert(cond_block)
            else_block = builder.create_block()
            self.else_br.generate()
            builder.add_connector(else_block, after_block)
            builder.set_insert(after_block)
        else:
            builder.add_connector(cond_block, after_block)
        return IR.IR.create_empty_expr()


class Ident:
    def __init__(self, name):
        # Конструктор класса Ident. Принимает имя переменной (name).
        self.name = name

    def __str__(self):
        # Возвращает строковое представление идентификатора.
        return self.name

    def generate(self):
        # Генерирует промежуточное представление (IR) для идентификатора.
        return self.name


class WhileExpr:
    def __init__(self, cond, body):
        # Конструктор класса WhileExpr. Принимает условие (cond) и тело цикла (body).
        self.cond = cond
        self.body = body

    def __str__(self):
        # Возвращает строковое представление цикла while.
        return "while (" + str(self.cond) + ") {" + str(self.body) + "}"

    def generate(self):
        # Генерирует промежуточное представление (IR) для цикла while.
        header = builder.create_block()
        cond_expr = IR.IR(IR.CMP, " ")
        e = self.cond.generate()
        if isinstance(e, BinOp):
            cond_expr.add_argument(e.l)
            cond_expr.add_argument(e.r)
            cond_expr.add_bin_op(e.op)
        else:
            cond_expr.add_argument(e)
        builder.add_expression(cond_expr)
        body = builder.create_block()
        self.body.generate()
        after_block = builder.create_block_without()
        builder.add_connector(body, header)
        builder.add_connector(header, after_block)
        return IR.IR.create_empty_expr()


class Number:
    def __init__(self, value):
        # Конструктор класса Number. Принимает значение числа (value).
        self.value = value

    def __str__(self):
        # Возвращает строковое представление числа.
        return str(self.value)

    def generate(self):
        # Генерирует промежуточное представление (IR) для числа.
        return str(self.value)


class Function:
    def __init__(self, body, return_expr):
        # Конструктор класса Function. Принимает тело функции (body) и выражение возврата (return_expr).
        self.body = body
        self.return_expr = return_expr

    def __str__(self):
        # Возвращает строковое представление функции.
        return "main {" + str(self.body) + str(self.return_expr) + "}"

    def generate(self):
        # Генерирует промежуточное представление (IR) для функции.
        builder.create_block()
        builder.create_block()
        self.body.generate()
        expr = IR.IR(IR.RETURN, " ")
        e = self.return_expr.generate()
        if isinstance(e, BinOp):
            expr.add_argument(e.l)
            expr.add_argument(e.r)
            expr.add_bin_op(e.op)
        else:
            expr.add_argument(e)
        builder.add_expression(expr)
        # the_context.graph.print_graph()
        builder.context.graph.dfs()
        builder.context.graph.build_dominators_tree()
        builder.context.graph.make_DF()
        builder.context.place_phi()
        builder.context.change_numeration()
        builder.context.graph.print_graph()

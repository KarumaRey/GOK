from graph import *

# Класс Context представляет контекст выполнения и хранит информацию о текущем состоянии процесса или алгоритма.
class Context:
    def __init__(self):
        self.count = 0  # Счетчик
        self.stack = []  # Стек
        self.n = 0  # Номер
        self.names = set()  # Множество имен
        self.graph = Graph()  # Граф
        self.cur_vertex = None  # Текущая вершина

    # Метод which_pred() определяет предшествующую вершину v1 в отношении вершины v.
    def which_pred(self, v1, v):
        for i, ver in enumerate(v1.input_vertexes):
            if ver.dfs_number == v.dfs_number:
                return i
        print("lose")

    # Метод change_numeration() изменяет нумерацию в графе для всех имен.
    def change_numeration(self):
        for name in self.names:
            self.count = 0
            self.stack.clear()
            self.change_numeration_by_name(name)

    # Метод change_numeration_by_name() изменяет нумерацию в графе для конкретного имени.
    def change_numeration_by_name(self, name):
        self.traverse(self.graph.vertexes[1], name)

    # Метод traverse() выполняет обход графа, изменяя нумерацию для конкретного имени.
    # переименование перемнной
    def traverse(self, v, name):
        for stmt in v.block:
            if stmt.type != IR.PHI:
                for i, arg in enumerate(stmt.arguments):
                    if arg[0] == name[0]:
                        stmt.arguments[i] = name + str(self.stack[-1])
            if stmt.value == name:
                stmt.value = stmt.value + str(self.count)
                self.stack.append(self.count)
                self.count += 1

        for succ in v.output_vertexes:
            j = self.which_pred(succ, v)
            for stmt in succ.block:
                if stmt.type == IR.PHI and stmt.value[0] == name[0]:
                    stmt.arguments[j] = name + str(self.stack[-1])
#дети каждого блока вызываемые для рекурсивного обхода
        for child in v.children:
            self.traverse(child, name)
        for stmt in v.block:
            l = stmt.value
            if l[0] == name[0]:
                self.stack.pop()

    # Метод place_phi() размещает операторы PHI в графе.
    def place_phi(self):
        for name in self.names:
            using_set = self.graph.get_all_assign(name)
            places = self.graph.make_dfp(using_set)
            for place in places:
                phi_expr = IR.IR(IR.PHI, name)
                for v in place.input_vertexes:
                    phi_expr.add_argument(name)
                place.insert_head(phi_expr)

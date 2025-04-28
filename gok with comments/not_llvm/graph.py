import IR


class Vertex:
    def __init__(self, block, input_vertexes, output_vertexes):
        # Конструктор класса Vertex, инициализирует объект вершины графа.
        self.block = block  # Блок выражений в вершине
        self.input_vertexes = input_vertexes  # Входящие вершины
        self.output_vertexes = output_vertexes  # Исходящие вершины
        self.idom = None  # Непосредственный доминатор вершины
        self.children = []  # Дочерние вершины
        self.number = None  # Номер вершины
        self.checked = False  # Флаг, указывающий, была ли вершина проверена
        self.dfs_number = None  # Номер вершины в обходе в глубину

    @staticmethod
    def init_empty_vertex():
        # Статический метод, создающий пустую вершину
        return Vertex([], [], [])

    def add_child(self, child):
        # Добавление дочерней вершины
        self.children.append(child)

    def insert_head(self, expr):
        # Вставка выражения в начало блока вершины
        self.block.insert(0, expr)

    def insert_tail(self, expr):
        # Вставка выражения в конец блока вершины
        self.block.append(expr)

    def add_output_connector(self, to):
        # Добавление исходящей связи (ребра) к другой вершине
        self.output_vertexes.append(to)

    def add_input_connector(self, from_v):
        # Добавление входящей связи (ребра) от другой вершины
        self.input_vertexes.append(from_v)

    def to_graph(self):
        # Вывод вершины в виде графического представления
        print("\t" + str(self.number) + "[label=\"")
        for elem in self.block:
            print("\t\t" + str(elem))
        print("\t\"]")
        for elem in self.output_vertexes:
            print("\t" + str(self.number) + "->" + str(elem.number))
        # for elem in self.children:
        #    print("\t" + str(self.number) + "->" + str(elem.number) + " [color = green]")


class Graph:
    def __init__(self):
        # Конструктор класса Graph, инициализирует объект графа.
        self.vertexes = []  # Список вершин графа
        self.DF = {}  # Множества DF для каждой вершины

    def add_vertex(self):
        # Добавление вершины в граф и возвращает созданную вершину
        vertex = Vertex.init_empty_vertex()
        self.vertexes.append(vertex)
        return vertex

    def make_DF(self):
        # Построение множеств DF для каждой вершины графа
        post_order = self.generate_post_order()
        for elem in post_order:
            self.DF[elem] = set()
            for succ in elem.output_vertexes:
                if succ.idom != elem:
                    self.DF[elem].add(succ)
            for child in elem.children:
                for v in self.DF[child]:
                    if v.idom != elem:
                        self.DF[elem].add(v)

    def dfs(self):
        # Обход графа в глубину для нумерации вершин
        n = 0
        first = self.vertexes[0]
        stack = [first]
        for v in self.vertexes:
            v.checked = False

        while len(stack) != 0:
            current = stack.pop()
            current.checked = True
            current.dfs_number = n
            n += 1
            for next in current.output_vertexes:
                if not next.checked:
                    stack.append(next)

    def print_graph(self):
        # Вывод графа в виде графического представления
        print("digraph g{")
        print("\tnode [shape = box]")
        for elem in self.vertexes:
            elem.to_graph()
        print("}")

    def generate_post_order(self):
        # Генерация списка вершин в порядке обхода в глубину
        for elem in self.vertexes:
            elem.checked = False
        return self.generate_post_order_from(self.vertexes[0])

    def generate_post_order_from(self, start):
        # Генерация списка вершин в порядке обхода в глубину, начиная с вершины start
        if len(start.output_vertexes) == 0:
            if start.checked:
                return []
            start.checked = True
            return [start]
        else:
            if start.checked:
                return []
            start.checked = True
            result = []
            for elem in start.output_vertexes:
                local_post_order = self.generate_post_order_from(elem)
                result += local_post_order
            result.append(start)
            return result

    def build_dominators_tree(self):
        # Построение дерева доминаторов
        first = self.vertexes[0]
        dominators = {}
        stack = []
        for elem in self.vertexes:
            dominators[elem] = []
        for elem in self.vertexes:
            for v in self.vertexes:
                v.checked = False
            elem.checked = True
            stack.append(first)
            while len(stack) != 0:
                current = stack.pop()
                current.checked = True
                for next in current.output_vertexes:
                    if not next.checked:
                        stack.append(next)
            for v in self.vertexes:
                if not v.checked:
                    dominators[v].append(elem)
        for elem in self.vertexes:
            maximum = None
            for v in dominators[elem]:
                if maximum is None or maximum.dfs_number < v.dfs_number:
                    maximum = v
            elem.idom = maximum
        for elem in self.vertexes:
            if elem.idom is not None:
                elem.idom.add_child(elem)

    def get_all_assign(self, s):
        # Получение всех вершин, содержащих оператор присваивания переменной s
        result = set()
        for elem in self.vertexes:
            for opp in elem.block:
                if opp.type == IR.ASSIGN and opp.value == s:
                    result.add(elem)
        return result

    def make_df_set(self, s):
        # Формирование множества DF для множества вершин s
        result = set()
        for elem in s:
            localResult = self.DF[elem]
            result = result.union(localResult)
        return result

    def make_dfp(self, s):
        # размещение ф-функций
        # Построение устойчивого множества вершин, достижимых из множества вершин s
        changed = True
        dfp = self.make_df_set(s)
        prev = dfp.copy()
        while changed:
            changed = False
            s = s.union(dfp)
            dfp = self.make_df_set(s)
            if dfp != prev:
                changed = True
                prev = dfp.copy()
        return dfp

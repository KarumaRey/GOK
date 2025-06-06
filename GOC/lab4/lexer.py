import re


class Token:
    def __init__(self, token_type, value, line, position):
        self.token_type = token_type
        self.value = value
        self.line = line
        self.position = position

    def __str__(self):
        return self.token_type + " " + self.value


class Lexer:
    def __init__(self, text):
        # Конструктор класса Lexer. Принимает исходный текст (text) для лексического анализа.
        self.text = text
        self.line = 1
        self.position = 1
        self.tokens = []
        self.rules = [
            ('WHILE', r'while'),
            ('IF', r'if'),
            ('ELSE', r'else'),
            ('L_FIG', r'[{]'),
            ('R_FIG', r'[}]'),
            ('LEFT_PAREN', r'[(]'),
            ('RIGHT_PAREN', r'[)]'),
            ('PLUS', r'[+]'),
            ('MINUS', r'[-]'),
            ('EQ', r'[=]'),
            ('RETURN', r'return'),
            ('IDENT', r'[A-Za-z][A-Za-z1-9]*'),
            ('NUMBER', r'[1-9][0-9]*'),
            ('END', r'\$'),
            ('SPACE', r'\s+')
        ]

    def tokenize(self):
        lines = self.text.splitlines()
        for line_num, line in enumerate(lines):
            self.position = 1
            self.line = line_num + 1
            while self.position < len(line) + 1:
                for rule in self.rules:
                    match = re.match(rule[1], line[self.position - 1:])
                    if match:
                        if rule[0] != 'SPACE':
                            value = match.group()
                            self.tokens.append(Token(rule[0], value, self.line, self.position))
                        self.position += match.end()
                        break
                else:
                    for token in self.tokens:
                        print(token)
                    print(f'lexer error ({self.line}, {self.position})\n')
                    raise ValueError

        return iter(self.tokens)


if __name__ == "__main__":
    with open('test.txt', 'r') as f:
        text = f.read()
    lexer = Lexer(text + "$")
    try:
        for token in lexer.tokenize():
            print(token.token_type, token.value)
    except ValueError as v:
        pass

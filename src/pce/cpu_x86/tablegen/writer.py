class Writer:
    def __init__(self, filename):
        self.indent_chars = 0
        self.file = open(filename, "w")
        self.write("// clang-format off")
        self.write_empty_line()

    def close(self):
        self.write_empty_line()
        self.write("// clang-format on")
        self.file.close()

    def indent(self):
        self.indent_chars += 2

    def deindent(self):
        assert (self.indent_chars > 0)
        self.indent_chars -= 2

    def begin_scope(self):
        self.write("{")
        self.indent()

    def end_scope(self):
        self.deindent()
        self.write("}")

    def write(self, text):
        self.file.write(' ' * self.indent_chars)
        self.file.write(text)
        self.file.write("\n")

    def write_empty_line(self):
        self.file.write("\n")
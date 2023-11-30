class String:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        str_len = int(self.val['len'])
        buf_string = gdb.selected_inferior().read_memory(self.val['buf'], str_len).tobytes().decode('utf-8', 'replace')
        return "{}".format(repr(buf_string), str_len)

def dispatch(val):
    if (val.type.code == gdb.TYPE_CODE_PTR):
        val = val.dereference()
    type_name = val.type.name
    if type_name == "String":
        return String(val)
gdb.pretty_printers.append(dispatch)

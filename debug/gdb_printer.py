class Namespace:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        res = "NS{"
        pairs = self.val['values']
        i = 0
        found = 0
        while (i < int(self.val['cap'])) and (found < int(self.val['len'])):
            if pairs[i]:
                found += 1
                res += String(pairs[i]['key']).to_string()
                # res += ": "
                # res += Elem(pairs[i]['elem']).to_string()
                if found < int(self.val['len']): res += " "
            i += 1
        res += "}"
        if self.val['parent']:
            res += "<-%s" % Namespace(self.val['parent'].dereference()).to_string()
        return res

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
    if type_name == "Namespace":
        return Namespace(val)
gdb.pretty_printers.append(dispatch)

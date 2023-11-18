# source me
class String:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        str_len = int(self.val['len'])
        buf_string = gdb.selected_inferior().read_memory(self.val['buf'], str_len).tobytes().decode('utf-8', 'replace')
        return "{}".format(repr(buf_string), str_len)

class List:
    def __init__(self, val, list_type):
        self.val = val
        self.type = list_type
    def to_string(self):
        res = "["
        ptr = self.val['head']
        while ptr:
            res += "%s" % ptr[self.type]
            if ptr['next']: res += " "
            ptr = ptr['next']
        res += "]"
        return res

class Ident:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        return "Id{%s}" % self.val['value']

class Elem:
    NULL = 0; ERR = 1; INT = 2; 
    STR = 3; BOOL = 4; LIST = 5;
    RETURN = 6; FUNC = 7; BUILTIN = 8;
    def __init__(self, val):
        self.val = val
    def to_string(self):
        if self.val['type'] == self.NULL:
            return "El|NULL|"
        if self.val['type'] == self.ERR:
            return "El|ERR, %s|" % self.val['_error']
        if self.val['type'] == self.INT:
            return "El|INT, %d|" % int(self.val['_int']['value'])
        if self.val['type'] == self.STR:
            return "El|STR, %s|" % self.val['_string']
        if self.val['type'] == self.BOOL:
            return "El|STR, %s|" % True if self.val['_bool']['value'] else False
        if self.val['type'] == self.LIST:
            return List(self.val["_list"]['items'], "el").to_string()
        if self.val['type'] == self.RETURN:
            return "El|RETURN, %s|" % self.val['_return']['value']
        if self.val['type'] == self.FUNC:
            return "El|FUNCTION, (%s), ns=%s|" % (List(self.val['_fn']['params'], "id").to_string(), self.val['_fn']['namespace'].address)
        if self.val['type'] == self.BUILTIN:
            return "El|BUILTIN, Not Impl|"
        return "Unkown Element"

class Pair:
    def __init__(self, val):
        self.val = val
    def to_string(self):
        res = String(self.val['key']).to_string()
        res += ": "
        res += Elem(self.val['elem']).to_string()
        return res

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
                res += ": "
                res += Elem(pairs[i]['elem']).to_string()
                if found < int(self.val['len']): res += " "
            i += 1
        res += "}"
        if self.val['global']:
            res += "<-%s" % Namespace(self.val['global'].dereference()).to_string()
        return res

def printer_dispatch(val):
    if (val.type.code == gdb.TYPE_CODE_PTR):
        val = val.dereference()
    type_name = val.type.name
    if type_name == "Element":
        return Elem(val)
    if type_name == "String":
        return String(val)
    if type_name == "Identifier":
        return Ident(val)
    if type_name == "Namespace":
        return Namespace(val)
    if type_name == "Pair":
        return Pair(val)
    return None

gdb.pretty_printers.append(printer_dispatch)

// May or may not use this in the future
priv ElemList *elemlist_from_array(Arena *a, ElemArray *arr)
{
	u64 previous_offset = a->used;
	ElemList *lst = elemlist(a);
	for (int i = 0; i < arr->len; i++) {
		if (NEVER(!arr->items + i))
			return (arena_pop_to(a, previous_offset), elemlist_single(a, error(str("Error copying array to list"))));
		elempush(lst, arr->items[i]);
	}
	return lst;
}
priv ElemArray *elemarray_from_list(Arena *a, Namespace *ns, ElemList *lst)
{
	u64 previous_offset = a->used;
	ElemArray *arr = elemarray(a, lst->len);
	ElemNode *tmp = lst->head;
	for (int i = 0; i < arr->len; i++) {
		if (NEVER(!tmp))
			return (arena_pop_to(a, previous_offset), elemarray_single(a, error(str("Error copying list to array"))));
		arr->items[i] = elem_copy(a, tmp->element);
		tmp = tmp->next;
	}
	return arr;
}


def generate_routes(nodes):
    id_to_prereq = {nid: [set(g) for g in prereqs if g] for nid, prereqs, _ in nodes}
    id_to_excl   = {nid: set(excl) for nid, _, excl in nodes}
    node_ids = [nid for nid, _, _ in nodes]

    def backtrack(route_list, route_set):
        yield route_list[:]

        for nid in node_ids:
            if nid in route_set:
                continue
            if route_set & id_to_excl[nid]:  # exclusivity conflict
                continue
            if any(len(g & route_set) == 0 for g in id_to_prereq[nid]):  # unmet prereq group
                continue

            route_list.append(nid)
            route_set.add(nid)
            yield from backtrack(route_list, route_set)
            route_list.pop()
            route_set.remove(nid)

    return list(backtrack([], set()))

nodes = [
    [0, [[]], []], 
    [1, [[0]], [2]],
    [2, [[0]], [1]],
    [3, [[1]], []],
    [4, [[2]], []],
    [5, [[3, 4]], []]
]


results = generate_routes(nodes)
results.pop(0)
print(len(results))
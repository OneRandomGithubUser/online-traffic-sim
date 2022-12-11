import osmium
import json
import uuid

nodes = {}
ways = {}

class NodeHandler(osmium.SimpleHandler):
    def __init__(self):
        
        osmium.SimpleHandler.__init__(self)
        self.num_nodes_n = 0

    def node(self, n):
        if "highway" in n.tags:
            self.num_nodes_n += 1
            nodes[str(n.id)] = {"uuid" : str(uuid.uuid4()), "x": n.location.lon, "y": n.location.lat}

class WayHandler(osmium.SimpleHandler):
    def __init__(self):
        
        osmium.SimpleHandler.__init__(self)
        self.num_nodes_w = 0
    def way(self, w):
        if "highway" in w.tags:
            newList = []
            for nodeID in w.nodes:
                if str(nodeID) in nodes:
                    newList.append(str(nodeID))
            if len(newList) > 1:
                ways[str(w.id)] = {"uuid" : str(uuid.uuid4()), "nodeIdList": newList}

if __name__ == '__main__':

    h = NodeHandler()
    g = WayHandler()

    filename = "map.osm"
    h.apply_file(filename)
    print("h done")
    g.apply_file(filename)
    for key, way in ways.items():
        way["nodeUuidList"] = []
        for nodeID in way["nodeIdList"]:
            way["nodeUuidList"].append(nodes[nodeID]["uuid"])
        way.pop("nodeIdList")
    with open('nodes.json', 'w', encoding='utf-8') as f:
        json.dump(list(nodes.values()), f)
    with open('ways.json', 'w', encoding='utf-8') as f:
        json.dump(list(ways.values()), f)

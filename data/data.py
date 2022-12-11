import osmium
import json
import uuid

nodes = {}
ways = {}
roadNodeIDs = []

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
        tags = w.tags.get("highway")
        if True or tags == "motorway" or tags == "trunk" or tags == "primary" or tags == "secondary":
            newList = []
            for nodeID in w.nodes:
                nodeIDSTring = str(nodeID)
                if nodeIDSTring in nodes:
                    newList.append(nodeIDSTring)
                    if nodeIDSTring not in roadNodeIDs:
                        roadNodeIDs.append(nodeIDSTring)
            if len(newList) > 1:
                ways[str(w.id)] = {"uuid" : str(uuid.uuid4()), "nodeIdList": newList}

if __name__ == '__main__':

    h = NodeHandler()
    g = WayHandler()

    filename = "maryland-latest.osm.pbf"
    h.apply_file(filename)
    print("h done")
    g.apply_file(filename)
    for key, way in ways.items():
        way["nodeUuidList"] = []
        for nodeID in way["nodeIdList"]:
            way["nodeUuidList"].append(nodes[nodeID]["uuid"])
        way.pop("nodeIdList")
    newNodeKeys = []
    for key, value in nodes.items():
        if key in roadNodeIDs:
            newNodeKeys.append(value)
    with open('nodes.json', 'w', encoding='utf-8') as f:
        json.dump(newNodeKeys, f)
    with open('ways.json', 'w', encoding='utf-8') as f:
        json.dump(list(ways.values()), f)

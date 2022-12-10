import osmium

class DataHandler(osmium.SimpleHandler):
    def __init__(self):
        osmium.SimpleHandler.__init__(self)
        self.roadNodesCoords = []

    def node(self, n):
        if "highway" in n.tags: #n.tags.get('highway')
            print(n.lat)
            self.roadNodesCoords.append((n.lat,n.lon))

if __name__ == '__main__':

    h = DataHandler()

    h.apply_file("map.osm", locations=True, idx='flex_mem')#maryland-latest.osm.pbf")

    print(h.roadNodesCoords)

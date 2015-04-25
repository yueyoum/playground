import json
import gzip
from cStringIO import StringIO

from protocol_pb2 import Person



class UnPack(object):
    def __init__(self):
        self.data_pb = open("data.pb").read()
        self.data_json = open("data.json").read()

        self.data_json_gzip_buf = StringIO(open("data.json.gz").read())

    def unpack_pb(self):
        msg = Person()  
        msg.ParseFromString(self.data_pb)
        return msg


    def unpack_json(self):
        return json.loads(self.data_json)


    def unpack_json_gzip(self):
        self.data_json_gzip_buf.seek(0)
        z = gzip.GzipFile(fileobj=self.data_json_gzip_buf)
        return json.loads(z.read())


if __name__ == '__main__':
    import sys
    import timeit

    if len(sys.argv) < 1:
        print "python unpack.py [beachmark times]"
        sys.exit(1)


    times = int(sys.argv[1])

    p = UnPack()

    pb_t = timeit.Timer("p.unpack_pb()", setup="from __main__ import p")
    json_t = timeit.Timer("p.unpack_json()", setup="from __main__ import p")
    json_gzip_t = timeit.Timer("p.unpack_json_gzip()", setup="from __main__ import p")

    print "Protobuf            :", pb_t.repeat(number=times)
    print "Json                :", json_t.repeat(number=times)
    print "Gzip Json           :", json_gzip_t.repeat(number=times)

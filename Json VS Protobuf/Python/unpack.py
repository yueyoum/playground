#!/usr/bin/env python
import os
import json
import gzip
from cStringIO import StringIO

from protocol_pb2 import Person



class UnPack(object):
    def __init__(self):
        project_path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        self.data_pb = open(os.path.join(project_path, "data.pb")).read()
        self.data_json = open(os.path.join(project_path, "data.json")).read()

        self.data_json_gzip_buf = StringIO(open(os.path.join(project_path, "data.json.gz")).read())

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

    try:
        times = int(sys.argv[1])
    except:
        print "./unpack.py [BENCHMARK TIMES]"
        sys.exit(1)


    p = UnPack()

    pb_t = timeit.Timer("p.unpack_pb()", setup="from __main__ import p")
    json_t = timeit.Timer("p.unpack_json()", setup="from __main__ import p")
    json_gzip_t = timeit.Timer("p.unpack_json_gzip()", setup="from __main__ import p")

    print "Benchmark Times =", times
    print "Protobuf Seconds  :", pb_t.timeit(number=times)
    print "Json Seconds      :", json_t.timeit(number=times)
    print "Json GZip Seconds :", json_gzip_t.timeit(number=times)

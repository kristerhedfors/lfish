#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Copyright(c) 2014 - Krister Hedfors
#
#
import os
import os.path
import sys
import tempfile
import urllib2
import pdb
import subprocess
import struct
import pickle
import config


#def _my_excepthook(type, value, tb):
#    import traceback
#    traceback.print_exception(type, value, tb)
#    pdb.pm()
#sys.excepthook = _my_excepthook


global DEBUG
DEBUG=os.environ.get('LFISH_DEBUG', False)


def debug(*args):
    if DEBUG:
        msg = '<{0}> '.format(os.getpid())
        msg += ' '.join(str(arg) for arg in args)
        sys.stderr.write(msg + '\n')
        sys.stderr.flush()


#
# fdebug from: http://paulbutler.org/archives/python-debugging-with-decorators/
#
__fdebug_indent = [0]

def fdebug(fn):
    def wrap(*params, **kwargs):
        call = wrap._count = wrap._count + 1

        indent = ' ' * __fdebug_indent[0]
        fc = "%s(%s)" % (fn.__name__, ', '.join(
            [a.__repr__() for a in params] +
            ["%s = %s" % (a, repr(b)) for a, b in kwargs.items()]
        ))

        debug("%s%s called [#%s]" % (indent, fc, call))
        __fdebug_indent[0] += 1
        ret = fn(*params, **kwargs)
        __fdebug_indent[0] -= 1
        debug("%s%s returned %s [#%s]" % (indent, fc, repr(ret), call))
        return ret
    wrap._count = 0
    return wrap


class WorkerException(Exception):
    pass


class WorkerBase(object):
    '''
        WorkerBase will lazily spawn a subprocess to execute tasks without
        LD_PRELOAD hooks.

        This is a workaround for crashes which occurr when then python open()
        hook handler issues new LIBC open() requests, which in turn triggers
        new python calls and so on. There may exist better solutions.
    '''

    _MAGIC_HEADER = '\xc9yn\x8f'

    def __init__(self, server=False):
        if not server:
            self._is_server = False
            self._p = None
        else:
            self._is_server = True
            self._infile = sys.stdin
            self._outfile = sys.stdout

    def _lazy_init(self):
        if self._p is None:
            self._p = self._spawn_server()
            self._infile = self._p.stdout
            self._outfile = self._p.stdin

    def _spawn_server(self):
        p = None
        args = [config.LFISH_ABSPATH, '--worker']
        if DEBUG:
            args.append('--debug')
        env = os.environ.copy()
        if 'LD_PRELOAD' in env:
            del env['LD_PRELOAD']
        try:
            p = subprocess.Popen(args, env=env,
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE)
        except:
            debug("FAILED subprocess", sys.exc_info())
            sys.exit(1)
        return p

    def _read_header(self):
        buf = self._infile.read(4)
        if self._is_server and len(buf) == 0:
            debug('SERVER exiting on length == 0')
            sys.exit()
        assert len(buf) == 4
        if buf != self._MAGIC_HEADER:
            raise WorkerException('invalid header: ' + repr(buf))

    def _read_length(self):
        buf = self._infile.read(4)
        assert len(buf) == 4
        length = struct.unpack('I', buf)[0]
        return length

    def _recv(self):
        debug('recv header...')
        self._read_header()
        debug('recv length...')
        length = self._read_length()
        debug('recv length done, recv data... ({0})'.format(length))
        buf = self._infile.read(length)
        debug('recv data done')
        data = pickle.loads(buf)
        return data

    def _send(self, data):
        buf = pickle.dumps(data)
        length = struct.pack('I', len(buf))
        debug('sending', data, repr(length+buf))
        pkt = self._MAGIC_HEADER + length + buf
        self._outfile.write(pkt)
        self._outfile.flush()
        debug('sending done')

    def mainloop(self):
        done = False
        while not done:
            req = self._recv()
            methname = req['methname']
            args = req['args']
            kw = req['kw']
            m = getattr(self, methname)
            res = m(*args, **kw)
            self._send(res)

    def _call(self, methname, *args, **kw):
        '''
            Call without reading response. Primarily for calling _exit().
        '''
        self._lazy_init()
        d = {}
        d['methname'] = methname
        d['args'] = args
        d['kw'] = kw
        self._send(d)

    def call(self, methname, *args, **kw):
        '''
            Used by client to invoke methods on server-side
        '''
        self._call(methname, *args, **kw)
        res = self._recv()
        return res

    def _exit(self):
        sys.exit()

    def terminate(self):
        self._call('_exit')
        self._p.wait()


class Worker(WorkerBase):

    PROXY = {
        #'https': 'https://192.168.2.128:8080/'
    }

    def _write_file(self, path, contents):
        f = open(path, 'w')
        f.write(contents)
        f.close()

    @fdebug
    def get_file_contents(self, pathname, savepath):
        data = open(pathname).read()
        self._write_file(savepath, data)
        return

    @fdebug
    def get_remote_file(self, pathname, savepath):
        '''
            for testing: ./testopen index.html
        '''
        proxy = urllib2.ProxyHandler(self.PROXY)
        self.opener = urllib2.build_opener(proxy)
        url = 'https://www.kernel.org/{0}'.format(pathname)
        f = self.opener.open(url)
        data = f.read()
        f.close()
        self._write_file(savepath, data)

    @fdebug
    def test(self, *args, **kw):
        return 'testresult-123'


class Hooks(object):

    def __init__(self):
        self._worker = Worker()

    @fdebug
    def do_hook_open_pathname(self, pathname):
        if pathname.startswith('/proc/'):
            return True
        if pathname.startswith('index.html'):
            return True
        return False

    @fdebug
    def open(self, pathname):
        fd, tmpname = tempfile.mkstemp(prefix='lfish_')
        self._worker.call("get_file_contents", pathname, tmpname)
        #self._worker.call("get_remote_file", pathname, tmpname)
        os.lseek(fd, 0, os.SEEK_SET)
        return fd



def _test():
    w = Worker()
    res = w.call('test', 'asd', 123, foo='bar')
    w.terminate()
    sys.exit()


if __name__ == '__main__':

    if '--debug' in sys.argv:
        DEBUG = True
        sys.argv.remove('--debug')

    if '--test' in sys.argv:
        sys.argv.remove('--test')
        _test()

    elif '--worker' in sys.argv:
        sys.argv.remove('--worker')
        Worker(server=True).mainloop()




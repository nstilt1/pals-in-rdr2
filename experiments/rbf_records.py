"""Original lossless RBF record codec; contains no game assets or schemas."""
from dataclasses import dataclass,field
from pathlib import Path
import struct,copy
@dataclass
class Node:
    name:str=''
    kind:int=0
    data:bytes=b''
    children:list=field(default_factory=list)
def read(raw):
    if raw[:4]!=b'RBF0':raise ValueError('not RBF')
    at=4;names=[];stack=[];root=None
    while at<len(raw):
        index,kind=raw[at:at+2];at+=2
        if index==255:
            assert kind==255 and stack
            stack.pop()
            if not stack:assert at==len(raw)
            continue
        if index==253:
            assert kind==255 and stack
            size=struct.unpack_from('<I',raw,at)[0];at+=4
            node=Node('',253,raw[at:at+size]);at+=size
            assert len(node.data)==size
        else:
            if index==len(names):
                size=struct.unpack_from('<H',raw,at)[0];at+=2
                names.append(raw[at:at+size].decode('ascii'));at+=size
            assert index<len(names)
            sizes={0:6,16:4,32:0,48:0,64:4,80:12}
            if kind==96:
                size=struct.unpack_from('<H',raw,at)[0]+2
            elif kind in sizes:size=sizes[kind]
            else:raise ValueError(f'unsupported RBF kind {kind:x} at {at}')
            node=Node(names[index],kind,raw[at:at+size]);at+=size
            assert len(node.data)==size
        if stack:stack[-1].children.append(node)
        else:
            assert root is None and node.kind==0
            root=node
        if node.kind==0:stack.append(node)
    assert not stack and root is not None
    return root
def write(root):
    raw=bytearray(b'RBF0');names={}
    def visit(n):
        if n.kind==253:raw.extend(b'\xfd\xff'+struct.pack('<I',len(n.data))+n.data);return
        if n.name not in names:
            assert len(names)<253
            index=len(names);names[n.name]=index
            name=n.name.encode('ascii');raw.extend(bytes([index,n.kind])+struct.pack('<H',len(name))+name)
        else:raw.extend(bytes([names[n.name],n.kind]))
        raw.extend(n.data)
        if n.kind==0:
            for c in n.children:visit(c)
            raw.extend(b'\xff\xff')
    visit(root);return bytes(raw)
def walk(n):
    yield n
    for c in n.children:yield from walk(c)
def text(n):
    if n is None:return ''
    if n.kind==253:return n.data.rstrip(b'\0').decode('utf8')
    if n.kind==96:return n.data[2:].rstrip(b'\0').decode('utf8')
    return ''.join(text(c) for c in n.children)
def child(n,name):return next((c for c in n.children if c.name==name),None)

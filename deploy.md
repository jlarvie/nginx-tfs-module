# pytfs #
## 本代码库为python-tfs-module，有1.3版本。最近更新2.0版本,详看代码的README ##
## pytfs在长期生产环境下使用过 ##
## 而nginx-tfs-module只适用于tfs-1.3版本，且只算是实验室版本 ##



#安装配置

**1)编译前先确定tfsclient已被安装。或[直接安装tfs](http://code.taobao.org/trac/tfs/wiki/deploy)**

且gcc能找到相关库 libtfsclient.so 与头文件， 本人在编译前，直接把库与头文件cp到/usr/lib, /usr/inlude下了。

**2)tfsclient源码是C++的，如编译有错，自己搜索nginx加入C++方式。.**

# 配置 #

```
#user root;
worker_processes 8;

events {
worker_connections 1024;
}

http {
include mime.types;
default_type application/octet-stream;

sendfile on;
keepalive_timeout 65;

server {
listen 80;
server_name localhost;

# 预知的最大上传文件大小
client_max_body_size 5m;
client_body_timeout 120;
# 每次读写tfs文件buffer大小
tfs_rb_buffer_size 2m;

location = /put {
tfs_put;
tfs_nsip '10.20.134.195:10000';
}

# 使用get方式传tfsname,如 curl localhost/get?tfsname=T1XXXXXXXXXXX
location = /get {
tfs_get;
tfs_nsip '10.20.134.195:10000';
}

error_page 500 502 503 504 /50x.html;
location = /50x.html {
root html;

}
}
}
```
# 代码库说明：pytfs #
## 本代码库只为pytfs服务，支持tfs-1.3版本、tfs-2.2.8版本(注意是pytfs支持2.0),详看代码的README ##
## pytfs在长期生产环境下使用过 ##
## 而nginx-tfs-module只适用于tfs-1.3版本，且只算是实验室版本 ##





**nginx\_http\_tfs\_module**

nginx module for tfs api

**tfs**
使用的tfs1.3版本，对更新版本不保证编译通过。
http://code.taobao.org/trac/tfs/wiki/ZhWikiStart


[nginx module参考资料](http://code.google.com/p/nginxsrp/wiki/NginxCodeReview)
[ngx\_tfs\_mods安装配置](http://code.google.com/p/nginx-tfs-module/wiki/deploy)

**BTW：本人在工作只使用pytfs作为工具，对nginx\_http\_tfs\_module并未认真使用，故其可用性未必达标。而pytfs基本无bug。**


# pytfs tfs的python 接口 #

2011-03-10 17:00
chuantong.huang@gmail.com

tfs 的python接口，编译出pytfs.so
**我只在2.5~2.7怎么测试通过，其它版本没试过**

把 setup.py 拷贝到tfs源码目录下,与Makefile同级
把 pytfs.cpp 拷贝到src/client/目录下

运行python setup.py install 即可安装pytfs

0.1版本，实现了以下接口：
```
import pytfs

tfs = pytfs.TfsClient() #Create a new TfsClient object.
tfs.tfs_init('127.0.0.1:10000') # 连接先nameserver

#0)
tfs.tfs_put('文件内容') # 新上传一文件到tfs, 返回tfsname
tfs.tfs_get('T1xxxxxxxxxx') # 下载一个文件，返回文件内容的str

#1)
tfs.tfs_open(None, pytfs.WRITE_MODE, None) # new open a file
tfs.tfs_write('文件内容1')
tfs.tfs_write('文件内容2')
tfs.tfs_close()
tfs.tfs_getname() # 返回刚刚上传完文件的tfsname

#2)
tfs.tfs_open('T1xxxxxxxxxxx', pytfs.READ_MODE, None) # new open a file
tfs.tfs_read() #读内容,返回全部文件容
tfs.tfs_close()

```

**另外，有朋友反应安装后不能import，提示找不xxx.so文件，建议朋友去搜索linux so文件查找、加载相关知识，我提示下以命令：ldconfig**
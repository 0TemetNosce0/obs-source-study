1qt打开Cmakelist.txt
2配置
设置BUILD_TESTS为true
设置QTDIR路径//用的是qt就不需要设置了
设置DepsPath路径 :eg F:/XXX/dependencies2015/win32
3编译
4拷贝构建目录下的rundir目录下的data，obs-plugin目录到构建目录下的UI目录下
拷贝rundir目录下的bin目录下的dll到构建目录下的UI目录下
5将拷贝过来data目录下的obs-plugin下的目录剪切到data目录下
================================================================